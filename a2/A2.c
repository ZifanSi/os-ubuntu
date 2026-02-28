/*
Compile: gcc -pthread A2.c -o A2
Run: ./A2 5
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define CHAIRS 3

/* shared variables */
int waiting = 0;
int chairs[CHAIRS];
int next_seat = 0;
int next_teach = 0;
int num_students = 0;
int ta_sleeping = 1;

/* sync objects */
pthread_mutex_t mutex;
sem_t students_waiting;
sem_t *student_called = NULL;
sem_t *student_done = NULL;

/* thread handles */
pthread_t ta_tid;
pthread_t *student_tids = NULL;
int *student_ids = NULL;

/* placeholder thread functions for now */
void *ta_work(void *arg) {
    return NULL;
}

void *student_work(void *arg) {
    return NULL;
}

/* free memory and destroy sync objects */
static void cleanup(void) {
    int i;

    pthread_mutex_destroy(&mutex);
    sem_destroy(&students_waiting);

    if (student_called != NULL) {
        for (i = 0; i < num_students; i++) {
            sem_destroy(&student_called[i]);
        }
    }

    if (student_done != NULL) {
        for (i = 0; i < num_students; i++) {
            sem_destroy(&student_done[i]);
        }
    }

    free(student_tids);
    free(student_ids);
    free(student_called);
    free(student_done);
}

int main(int argc, char *argv[]) {
    int i;

    /* =========================
       A. Setup
       ========================= */

    /* a1. Read number of students from command line */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_students>\n", argv[0]);
        return 1;
    }

    num_students = atoi(argv[1]);
    if (num_students <= 0) {
        fprintf(stderr, "Number of students must be greater than 0.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    /* a2. Initialize shared variables */
    waiting = 0;
    next_seat = 0;
    next_teach = 0;
    ta_sleeping = 1;

    for (i = 0; i < CHAIRS; i++) {
        chairs[i] = -1;
    }

    /* a3. Initialize mutex and semaphores */
    pthread_mutex_init(&mutex, NULL);
    sem_init(&students_waiting, 0, 0);

    /* a4. Allocate arrays for threads, ids, semaphores */
    student_tids = malloc(num_students * sizeof(pthread_t));
    student_ids = malloc(num_students * sizeof(int));
    student_called = malloc(num_students * sizeof(sem_t));
    student_done = malloc(num_students * sizeof(sem_t));

    if (student_tids == NULL || student_ids == NULL ||
        student_called == NULL || student_done == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(student_tids);
        free(student_ids);
        free(student_called);
        free(student_done);
        return 1;
    }

    /* one called semaphore and one done semaphore per student */
    for (i = 0; i < num_students; i++) {
        sem_init(&student_called[i], 0, 0);
        sem_init(&student_done[i], 0, 0);
    }

    /* =========================
       B. Create threads
       ========================= */

    /* b1. Create 1 TA thread */
    if (pthread_create(&ta_tid, NULL, ta_work, NULL) != 0) {
        fprintf(stderr, "Could not create TA thread.\n");
        cleanup();
        return 1;
    }

    /* b2. Create n student threads */
    /* b3. Give each student a unique id */
    for (i = 0; i < num_students; i++) {
        student_ids[i] = i + 1;

        if (pthread_create(&student_tids[i], NULL, student_work, &student_ids[i]) != 0) {
            fprintf(stderr, "Could not create student thread %d.\n", i + 1);
            cleanup();
            return 1;
        }
    }

    /* joins added so structure is complete */
    pthread_join(ta_tid, NULL);

    for (i = 0; i < num_students; i++) {
        pthread_join(student_tids[i], NULL);
    }

    cleanup();
    return 0;
}