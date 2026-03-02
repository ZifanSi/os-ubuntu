/*
Compile: gcc -pthread A2.c -o A2
Run: ./A2 5
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHAIRS 3

/* log objects */
FILE *log_fp = NULL;
pthread_mutex_t log_mutex;

/* log function: writes to terminal and log file */
void log_printf(const char *fmt, ...) {
    va_list args;

    pthread_mutex_lock(&log_mutex);

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);

    if (log_fp != NULL) {
        va_start(args, fmt);
        vfprintf(log_fp, fmt, args);
        va_end(args);
        fflush(log_fp);
    }

    pthread_mutex_unlock(&log_mutex);
}

/* redirect all printf calls to log_printf */
#define printf(...) log_printf(__VA_ARGS__)

/* shared variables */
int waiting = 0;
int chairs[CHAIRS];
int next_seat = 0;
int next_teach = 0;
int num_students = 0;

int ta_sleeping = 1;
int ta_busy = 0;
int current_student = 0;   /* student getting immediate help */

/* sync objects */
pthread_mutex_t mutex;
sem_t students_waiting;
sem_t *student_called = NULL;
sem_t *student_done = NULL;

/* thread handles */
pthread_t ta_tid;
pthread_t *student_tids = NULL;
int *student_ids = NULL;

/* random number in [low, high] */
int rand_range(int low, int high) {
    return low + rand() % (high - low + 1);
}

/* student programming time */
void program_time(int id) {
    int t = rand_range(1, 5);
    printf("Student %d is programming for %d seconds.\n", id, t);
    sleep(t);
}

/* TA helping time */
void help_time(int id) {
    int t = rand_range(1, 3);
    printf("TA is helping student %d for %d seconds.\n", id, t);
    sleep(t);
    printf("TA finished helping student %d.\n", id);
}

/* TA thread */
void *ta_work(void *arg) {
    int id;

    (void)arg;

    while (1) {
        pthread_mutex_lock(&mutex);
        if (!ta_busy && waiting == 0 && current_student == 0) {
            ta_sleeping = 1;
            printf("TA is sleeping.\n");
        }
        pthread_mutex_unlock(&mutex);

        /* wait until some student needs help */
        sem_wait(&students_waiting);

        pthread_mutex_lock(&mutex);

        ta_sleeping = 0;

        /* immediate-help student gets priority if one exists */
        if (current_student != 0) {
            id = current_student;
            current_student = 0;
            printf("TA starts helping student %d immediately.\n", id);
        } else {
            id = chairs[next_teach];
            chairs[next_teach] = -1;
            next_teach = (next_teach + 1) % CHAIRS;
            waiting--;

            printf("TA calls student %d. Waiting students left: %d\n", id, waiting);
        }

        pthread_mutex_unlock(&mutex);

        /* call the correct student */
        sem_post(&student_called[id - 1]);

        /* help that student */
        help_time(id);

        /* tell student help is done */
        sem_post(&student_done[id - 1]);

        pthread_mutex_lock(&mutex);
        ta_busy = 0;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

/* student thread */
void *student_work(void *arg) {
    int id = *(int *)arg;

    while (1) {
        /* c1. student programs first */
        program_time(id);

        /* c2. student asks for help */
        pthread_mutex_lock(&mutex);

        /* if TA is free and nobody is already waiting, get immediate help */
        if (!ta_busy && waiting == 0 && current_student == 0) {
            ta_busy = 1;
            current_student = id;

            if (ta_sleeping) {
                printf("Student %d wakes up the TA and gets immediate help.\n", id);
            } else {
                printf("Student %d finds TA available and gets immediate help.\n", id);
            }

            pthread_mutex_unlock(&mutex);

            /* notify TA */
            sem_post(&students_waiting);

            /* wait until TA calls this student */
            sem_wait(&student_called[id - 1]);
            printf("Student %d goes into the office for help.\n", id);

            /* wait until help is finished */
            sem_wait(&student_done[id - 1]);
            printf("Student %d leaves the office.\n", id);
        }
        /* otherwise, sit in a waiting chair if available */
        else if (waiting < CHAIRS) {
            chairs[next_seat] = id;
            printf("Student %d sits in chair %d.\n", id, next_seat);

            next_seat = (next_seat + 1) % CHAIRS;
            waiting++;

            printf("Student %d is waiting. Total waiting: %d\n", id, waiting);

            pthread_mutex_unlock(&mutex);

            /* notify TA that a student is waiting */
            sem_post(&students_waiting);

            /* wait until TA calls this student */
            sem_wait(&student_called[id - 1]);
            printf("Student %d goes into the office for help.\n", id);

            /* wait until help is finished */
            sem_wait(&student_done[id - 1]);
            printf("Student %d leaves the office.\n", id);
        } else {
            /* no chair, come back later */
            printf("Student %d found no empty chair and will come back later.\n", id);
            pthread_mutex_unlock(&mutex);
        }
    }

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

    if (log_fp != NULL) {
        fclose(log_fp);
    }

    pthread_mutex_destroy(&log_mutex);
}

int main(int argc, char *argv[]) {
    int i;

    /* create log folder and log file */
    mkdir("logs", 0777);
    log_fp = fopen("logs/ta_output.log", "a");
    if (log_fp == NULL) {
        fprintf(stderr, "Could not open log file.\n");
        return 1;
    }

    pthread_mutex_init(&log_mutex, NULL);

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
    ta_busy = 0;
    current_student = 0;

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

    /* this version runs forever */
    pthread_join(ta_tid, NULL);

    for (i = 0; i < num_students; i++) {
        pthread_join(student_tids[i], NULL);
    }

    cleanup();
    return 0;
}