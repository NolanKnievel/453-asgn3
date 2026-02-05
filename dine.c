#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 3
#endif

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif


// philosopher struct
typedef struct {
    int state; // 0=changing, 1=eating, 2=thinking
    int has_left;
    int has_right;
} philosopher_st;

static philosopher_st philosophers[NUM_PHILOSOPHERS];
static sem_t forks[NUM_PHILOSOPHERS]; // forks - sempahores
static sem_t print_lock; // semaphore to keep printing in sync

static int cycles = 1;

/* ---------- Utilities ---------- */

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


void dawdle() {
/*
* sleep for a random amount of time between 0 and DAWDLEFACTOR
* milliseconds.
*/
    struct timespec tv;
    int msec = (int)((((double)random()) / RAND_MAX) * DAWDLEFACTOR);
    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec;

    if ( -1 == nanosleep(&tv,NULL) ) {
        perror("nanosleep");
    }
}


// helper function to print the header
void print_header() {
    int i;
    char equal_chars_str[NUM_PHILOSOPHERS+19];
    char equal_space_str[NUM_PHILOSOPHERS+5];

    equal_chars_str[NUM_PHILOSOPHERS+18] = '\0';
    for(i=0; i<NUM_PHILOSOPHERS+18; i++) {
        equal_chars_str[i] = '=';
    }

    equal_space_str[NUM_PHILOSOPHERS+4] = '\0';
    for(i=0; i<NUM_PHILOSOPHERS+4; i++) {
        equal_space_str[i] = ' ';
    }



    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("%s|", equal_chars_str);
    printf("\n|");

    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf(" %-*c |", NUM_PHILOSOPHERS, 'A' + i);
    printf("\n|");

    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("%s|", equal_chars_str);
    printf("\n");
}

// helper function - prints row of philosopehr states
void print_row() {
    int i;
    int j;
    char forks_str[NUM_PHILOSOPHERS + 1];
    forks_str[NUM_PHILOSOPHERS] = '\0';


    
    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        // initialize forks string
        for(j=0; j<NUM_PHILOSOPHERS; j++) {
            forks_str[j] = '-';
        }

        if (philosophers[i].has_left)
            forks_str[i] = '0' + i;
        if (philosophers[i].has_right)
            forks_str[(i + 1) % NUM_PHILOSOPHERS] = '0' + (i + 1) % NUM_PHILOSOPHERS;
    
        // print forks string
        printf(" %*s", NUM_PHILOSOPHERS, forks_str);

        // print state
        if (philosophers[i].state == 1)
            printf(" Eat   |");
        else if (philosophers[i].state == 2)
            printf(" Think |");
        else
            printf("       |");
    }
    printf("\n");
}


// helper to keep printing synchronized
void status_change() {
    sem_wait(&print_lock);
    print_row();
    sem_post(&print_lock);
}

// helper - wait for fork and take semaphore, print new row to show action
void pickup_fork(int phil, int fork, int is_left) {
    sem_wait(&forks[fork]);
    sem_wait(&print_lock);
    if (is_left)
        philosophers[phil].has_left = 1;
    else
        philosophers[phil].has_right = 1;
    print_row();
    sem_post(&print_lock);
}

// helper - release semaphore, print new row to show action
void put_down_fork(int phil, int fork, int is_left) {
    sem_post(&forks[fork]);
    sem_wait(&print_lock);
    if (is_left)
        philosophers[phil].has_left = 0;
    else
        philosophers[phil].has_right = 0;
    print_row();
    sem_post(&print_lock);
}

/* ---------- Philosopher Thread ---------- */

void *philosopher(void *arg) {
    int id;
    int left;
    int right;
    int i;

    // get id
    id = *(int *)arg;

    // left and right fork locations
    left = id;
    right = (id + 1) % NUM_PHILOSOPHERS;


    // start of eat-think cycle
    for (i = 0; i < cycles; i++) {
        // hungry - changing
        philosophers[id].state = 0;
        status_change();

        // avoid deadlock
        // odds pickup left fork first, evens right fork
        if (id % 2 == 0) {
            pickup_fork(id, right, 0);
            pickup_fork(id, left, 1);
        } else {
            pickup_fork(id, left, 1);
            pickup_fork(id, right, 0);
        }

        // eating
        philosophers[id].state = 1;
        status_change();
        // dawdle while eating
        dawdle();

        // changing
        philosophers[id].state = 0;
        status_change();

        put_down_fork(id, left, 1);
        put_down_fork(id, right, 0);

        // thinking
        philosophers[id].state = 2;
        status_change();
        // dawdle while thinking
        dawdle();
    }

    // one last print
    philosophers[id].state = 0;
    status_change();
    return NULL;
}

/* ---------- Main ---------- */

int main(int argc, char *argv[]) {
    int i; // for loops
    pthread_t tids[NUM_PHILOSOPHERS]; // pthread ids
    int ids[NUM_PHILOSOPHERS]; // philosopher ids

    // check arg count
    if (argc > 2) {
        fprintf(stderr, "usage: %s [cycles]\n", argv[0]);
        return -1;
    }
    
    // get cycles if provided
    if (argc == 2) {
        cycles = atoi(argv[1]);
        if (cycles <= 0) {
            fprintf(stderr, "cycles must be positive\n");
            return -1;
        }
    }

    // initialize forks
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        if (sem_init(&forks[i], 0, 1) != 0)
            die("sem_init");

    // initialize print lock
    if (sem_init(&print_lock, 0, 1) != 0)
        die("sem_init");

    // initialize philosopher structs
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].state = 0;
        philosophers[i].has_left = 0;
        philosophers[i].has_right = 0;
    }

    // print out start
    print_header();
    status_change();

    // spin up phil threads passing id as arg
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        if (pthread_create(&tids[i], NULL, philosopher, &ids[i]) != 0)
            die("pthread_create");
    }

    // wait for all threads to finish
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_join(tids[i], NULL);

    // print bottom
    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("=============|");
    printf("\n");

    // destroy sempahores
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        sem_destroy(&forks[i]);
    sem_destroy(&print_lock);

    return 0;
}
