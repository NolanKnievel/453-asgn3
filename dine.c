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
#define NUM_PHILOSOPHERS 5
#endif

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif

typedef enum {
    STATE_CHANGING,
    STATE_EATING,
    STATE_THINKING
} state_t;

typedef struct {
    state_t state;
    int has_left;
    int has_right;
} philosopher_t;

static philosopher_t philosophers[NUM_PHILOSOPHERS];
static sem_t forks[NUM_PHILOSOPHERS];
static sem_t print_lock;

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

int left_fork(int i) {
    return i;
}

int right_fork(int i) {
    return (i + 1) % NUM_PHILOSOPHERS;
}

/* ---------- Printing ---------- */

void print_header() {
    int i;

    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("=============|");
    printf("\n|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf(" %c           |", 'A' + i);
    printf("\n|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("=============|");
    printf("\n");
}

void print_row() {
    int i;
    char forks_str[6];

    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        strcpy(forks_str, "-----");

        if (philosophers[i].has_left)
            forks_str[left_fork(i)] = '0' + left_fork(i);
        if (philosophers[i].has_right)
            forks_str[right_fork(i)] = '0' + right_fork(i);

        printf(" %-5s", forks_str);

        if (philosophers[i].state == STATE_EATING)
            printf(" Eat     |");
        else if (philosophers[i].state == STATE_THINKING)
            printf(" Think     |");
        else
            printf("         |");
    }
    printf("\n");
}

void status_change() {
    sem_wait(&print_lock);
    print_row();
    sem_post(&print_lock);
}

/* ---------- Fork Ops ---------- */

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

void putdown_fork(int phil, int fork, int is_left) {
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

    id = *(int *)arg;
    left = left_fork(id);
    right = right_fork(id);

    for (i = 0; i < cycles; i++) {
        /* Hungry -> changing */
        philosophers[id].state = STATE_CHANGING;
        status_change();

        /* Deadlock avoidance */
        if (id % 2 == 0) {
            pickup_fork(id, right, 0);
            pickup_fork(id, left, 1);
        } else {
            pickup_fork(id, left, 1);
            pickup_fork(id, right, 0);
        }

        philosophers[id].state = STATE_EATING;
        status_change();
        dawdle();

        philosophers[id].state = STATE_CHANGING;
        status_change();

        putdown_fork(id, left, 1);
        putdown_fork(id, right, 0);

        philosophers[id].state = STATE_THINKING;
        status_change();
        dawdle();
    }

    philosophers[id].state = STATE_CHANGING;
    status_change();
    return NULL;
}

/* ---------- Main ---------- */

int main(int argc, char *argv[]) {
    int i;
    pthread_t tids[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];

    if (argc > 2) {
        fprintf(stderr, "usage: %s [cycles]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        cycles = atoi(argv[1]);
        if (cycles <= 0) {
            fprintf(stderr, "cycles must be positive\n");
            return EXIT_FAILURE;
        }
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        if (sem_init(&forks[i], 0, 1) != 0)
            die("sem_init");

    if (sem_init(&print_lock, 0, 1) != 0)
        die("sem_init");

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].state = STATE_CHANGING;
        philosophers[i].has_left = 0;
        philosophers[i].has_right = 0;
    }

    print_header();
    status_change();

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        ids[i] = i;
        if (pthread_create(&tids[i], NULL, philosopher, &ids[i]) != 0)
            die("pthread_create");
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        pthread_join(tids[i], NULL);

    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        printf("=============|");
    printf("\n");

    for (i = 0; i < NUM_PHILOSOPHERS; i++)
        sem_destroy(&forks[i]);
    sem_destroy(&print_lock);

    return 0;
}
