#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_CHILDREN 4



void *child(void *id) {
    int whoami = *(int *)id;
    printf("Child %d (%d): Hello.\n\n", whoami, (int) getpid());
    printf("Child %d (%d): Goodbye.\n\n", whoami, (int) getpid());
    return NULL;

}


int main() {
    pid_t ppid;
    int i;

    int id[NUM_CHILDREN];
    pthread_t childid[NUM_CHILDREN];

    // initialize ids array
    for(i=0; i<NUM_CHILDREN; i++) {
        id[i] = i;
    }

    // parent process id
    ppid = getpid();

    for(i=0; i<NUM_CHILDREN; i++) {
        int res;
        res = pthread_create(
            &childid[i],
            NULL,
            child,
            (void *) &id[i]
        );
    
        // report error if there was one
        if (-1 == res) {
            fprintf(stderr, "Child %i: %s\n",i,strerror(res));
            exit(-1);

        }
    }


    printf("Parent (%d): Hello. \n\n", (int) ppid);

    // wait for ach child to finish
    for(i=0; i<NUM_CHILDREN; i++) {
        pthread_join(childid[i], NULL);
        printf("Parent (%d): child %d exited.\n\n", (int) ppid, i);

    }

    printf("Parent (%d): Goodbye. \n\n", (int) ppid);
    return 0;

}
