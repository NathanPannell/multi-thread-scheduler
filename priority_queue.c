#include "shared.h"


typedef struct Node {
    TrainInfo *data;
    struct Node *next;
} Node;


Node *heads[NUM_DIRECTIONS];
pthread_mutex_t queue_mutexes[NUM_DIRECTIONS];


void init_pq() {
    for (int i = 0; i < NUM_DIRECTIONS; ++i) {
        pthread_mutex_init(&queue_mutexes[i], NULL);
        heads[i] = NULL;
    }
}


void destroy_pq() {
    for (int i = 0; i < NUM_DIRECTIONS; ++i) {
        pthread_mutex_destroy(&queue_mutexes[i]);
    }
}


// Priority > load_time > index
int compare(void *x, void *y) {
    TrainInfo *a = (TrainInfo *)x;
    TrainInfo *b = (TrainInfo *)y;

    if (a->priority != b->priority) return b->priority - a->priority;
    if (a->load_time != b->load_time) return a->load_time - b->load_time;
    return a->index - b->index;
}


void insert(TrainInfo *elem) {
    pthread_mutex_lock(&queue_mutexes[elem->direction]);
    
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "malloc: Unable to allocate memory for new node.\n");
        exit(1);
    }
    new_node->data = elem;
    new_node->next = NULL;

    // Sort new node into queue
    Node *head = heads[elem->direction];
    if (head == NULL || compare(elem, head->data) < 0) {
        new_node->next = head;
        head = new_node;
    } else {  
        Node *current = head;
        while (current->next != NULL && compare(current->next->data, elem) < 0) {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }

    // Overwrite previous (global) head of queue
    heads[elem->direction] = head;

    pthread_mutex_unlock(&queue_mutexes[elem->direction]);
}


// Return 'head' value without modification to queue
TrainInfo *find_min(int direction) {
    pthread_mutex_lock(&queue_mutexes[direction]);

    Node *head = heads[direction];
    TrainInfo *minElem = (head != NULL) ? head->data : NULL;

    pthread_mutex_unlock(&queue_mutexes[direction]);
    return minElem;
}


void remove_min(int direction) {
    pthread_mutex_lock(&queue_mutexes[direction]);

    Node *head = heads[direction];
    if (head != NULL) {
        heads[direction] = head->next;
        free(head);
    }

    pthread_mutex_unlock(&queue_mutexes[direction]);
}


// For debugging purposes
void print_queue(int direction) {
    printf("%s Queue:\n\n", direction == EAST ? "East" : "West");
    Node *head = heads[direction];
    while (head != NULL) {
        printf("%d\n", head->data->index);
        head = head->next;
    }
    printf("\n");
}