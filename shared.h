#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <search.h>


#define MAX_THREADS 100
#define NUM_DIRECTIONS 2
#define MICROSECONDS_PER_DECISECOND 100000
#define NANOSECONDS_PER_SECOND 1000000000


enum ThreadStatus {
    WAITING = 0,
    LOADING,
    READY,
    GRANTED,
    CROSSING,
    FINISHED
};


enum Direction {
    WEST = 0,
    EAST
};


enum Priority {
    LOW = 0,
    HIGH
};


typedef struct {
    int priority;
    int direction;
    int load_time;
    int cross_time;
    int index;
} TrainInfo;


// Priority Queue Implementation
void init_pq();
void destroy_pq();
void print_queue(int direction);
void insert(TrainInfo *elem);
void remove_min(int direction);
TrainInfo *find_min(int direction);