#include "shared.h"


pthread_cond_t worker_status = PTHREAD_COND_INITIALIZER;
pthread_mutex_t worker_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_load = PTHREAD_COND_INITIALIZER;
pthread_mutex_t start_load_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cross_track = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cross_track_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;


int thread_statuses[MAX_THREADS];
int next_status_index = 0;
int all_trains_created = 0;
int is_track_busy = 0;
struct timespec start;


// 0 -> All trains are finished
// 1 -> Not all trains are finished
int trains_status(int *status, int len) {
    for (int i = 0; i < len; i++) {
        if (status[i] != FINISHED) { 
            return 1; 
        }
    }
    return 0;
}


// For debugging purposes
void print_status(int *status, int len) {
    for (int i = 0; i < len; i++) {
        printf("%d: %d\n", i, status[i]);
    }
}


// Prints formatted time since timer start
void print_time(struct timespec *start) {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    long elapsed_ns = (end.tv_sec - start->tv_sec) * 1000000000L + (end.tv_nsec - start->tv_nsec);
    long elapsed_ds = (elapsed_ns + 50000000) / 100000000; // Add half a decisecond for rounding
    int tenths = elapsed_ds % 10;
    int seconds = (elapsed_ds / 10) % 60;
    int minutes = (elapsed_ds / 600) % 60;
    int hours = (elapsed_ds / 36000) % 24;

    printf("%02d:%02d:%02d.%1d ", hours, minutes, seconds, tenths);
}


// Primary worker thread: Representing a train
void *new_train(void *arg) {
    TrainInfo *info = (TrainInfo *)arg;
    int index = info->index;
    int cross_time = info->cross_time;
    char *direction = info->direction == EAST ? "East" : "West";
    
    // Wait for synchronized loading start
    pthread_mutex_lock(&start_load_mutex);
    while (!all_trains_created) {
        pthread_cond_wait(&start_load, &start_load_mutex);
    }
    pthread_mutex_unlock(&start_load_mutex);

    // Loading
    clock_gettime(CLOCK_MONOTONIC, &start);
    thread_statuses[index] = LOADING;
    usleep(info->load_time * MICROSECONDS_PER_DECISECOND);

    pthread_mutex_lock(&output_mutex);
    print_time(&start);
    printf("Train %2d is ready to go %4s\n", index, direction);
    pthread_mutex_unlock(&output_mutex);

    // Add to Priority Queue
    insert(info);
    thread_statuses[index] = READY;

    // Wait for signal to cross
    pthread_mutex_lock(&worker_status_mutex);
    while (thread_statuses[index] != GRANTED) {
        pthread_cond_wait(&worker_status, &worker_status_mutex);
    }
    pthread_mutex_unlock(&worker_status_mutex);

    // Crossing
    pthread_mutex_lock(&cross_track_mutex);
    thread_statuses[index] = CROSSING;

    pthread_mutex_lock(&output_mutex);
    print_time(&start);
    printf("Train %2d is ON the main track going %4s\n", index, direction);
    pthread_mutex_unlock(&output_mutex);

    usleep(cross_time * MICROSECONDS_PER_DECISECOND);

    pthread_mutex_lock(&output_mutex);
    print_time(&start);
    printf("Train %2d is OFF the main track after going %4s\n", index, direction);
    pthread_mutex_unlock(&output_mutex);

    thread_statuses[index] = FINISHED;
    is_track_busy = 0;
    pthread_cond_signal(&cross_track);
    pthread_mutex_unlock(&cross_track_mutex);

    return NULL;
}


// Creating threads from input file
void create_threads(FILE *file) {
    size_t len = 0;
    ssize_t read;
    pthread_t tid;
    char *line = NULL;
    char *token = NULL;
    while ((read = getline(&line, &len, file)) != -1) {
        line[read - 1] = '\0';
        TrainInfo *params = malloc(sizeof(TrainInfo));
        
        token = strtok(line, " ");
        if (token == NULL) {
            fprintf(stderr, "Missing station information.\n");
            exit(1);
        }
        
        if (strncmp(token, "E", 1) == 0 || strncmp(token, "e", 1) == 0) {
            params->direction = EAST;
        } else if (strncmp(token, "W", 1) == 0 || strncmp(token, "w", 1) == 0) {
            params->direction = WEST;
        } else {
            fprintf(stderr, "Unknown direction '%s'.\n", token);
            exit(1);
        }

        if (strncmp(token, "E", 1) == 0 || strncmp(token, "W", 1) == 0) {
            params->priority = HIGH;;
        } else if (strncmp(token, "e", 1) == 0 || strncmp(token, "w", 1) == 0) {
            params->priority = LOW;
        } 

        token = strtok(NULL, " ");
        if (token == NULL) {
            fprintf(stderr, "Missing load time information.\n");
            exit(1);
        }
        params->load_time = atoi(token);

        token = strtok(NULL, " ");
        if (token == NULL) {
            fprintf(stderr, "Missing crossing time information.\n");
            exit(EXIT_FAILURE);
        }
        params->cross_time = atoi(token);

        params->index = next_status_index++;
        thread_statuses[params->index] = WAITING;

        if (pthread_create(&tid, NULL, new_train, params) != 0) {
            perror("pthread_create");
            free(params);
        }
    }
    free(line);
}


// Error handling for command line arguments
// Safe file opening
FILE *open_file(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "fopen: Unable to open file '%s'\n", argv[1]);
        exit(1);
    }

    return file;
}


int main(int argc, char *argv[]) {

    FILE *file = open_file(argc, argv);
    create_threads(file);
    init_pq();

    // Broadcast: Ready to start loading for all threads
    pthread_mutex_lock(&start_load_mutex);
    all_trains_created = 1;
    pthread_cond_broadcast(&start_load);
    pthread_mutex_unlock(&start_load_mutex);

    // >0 # of east trains in a row
    // <0 # of west trains in a row
    int starvation = 0;
    int signal = 0;
    usleep(10000); // Sleep for 0.01 seconds to fix synchronization errors
    while (trains_status(thread_statuses, next_status_index)) {

        // Wait until track is clear to evaluate options
        pthread_mutex_lock(&cross_track_mutex);
        while(is_track_busy) {
            pthread_cond_wait(&cross_track, &cross_track_mutex);
        }
        pthread_mutex_unlock(&cross_track_mutex);

        // Find highest priority train in each direction
        TrainInfo *next_east = find_min(EAST);
        TrainInfo *next_west = find_min(WEST);

        /*
            Conductor logic
            1. No Trains -> no signal
            2. Only one side -> signal that side
            3. If available on both sides, check for starvation case
            4. Order by priority > direction > load time
                I.e. Two trains with identical priority will decide based on last direction sent
        */
        if (next_east == NULL && next_west == NULL) { 
            usleep(100000);
            continue; 
        } 
        else if (next_east == NULL) { signal = WEST; } 
        else if (next_west == NULL) { signal = EAST; } 
        else {
            if (starvation >= 3) { signal = WEST; } 
            else if (starvation <= -3) { signal = EAST; }
            else {
                if (next_east->priority != next_west->priority) { signal = next_east->priority > next_west->priority ? EAST : WEST; } 
                else { signal = starvation < 0 ? EAST : WEST; }
            }
        }
        
        // Remove selected train from queue
        // Signal train to start crossing
        remove_min(signal);
        if (signal == EAST) {
            thread_statuses[next_east->index] = GRANTED;
            starvation = starvation > 0 ? starvation + 1 : 1;
        } else {
            thread_statuses[next_west->index] = GRANTED;
            starvation = starvation < 0 ? starvation - 1 : -1;
        }

        pthread_mutex_lock(&cross_track_mutex);
        is_track_busy = 1;
        pthread_cond_broadcast(&worker_status);
        pthread_mutex_unlock(&cross_track_mutex);
    }

    fclose(file);
    destroy_pq();

    return 0;
}
