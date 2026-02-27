/*
 * A2.c — Sleeping Teaching Assistant (POSIX threads + mutex + semaphores)
 *
 * Build:
 *   gcc -O2 -Wall -Wextra -pthread A2.c -o A2
 *
 * Run:
 *   ./A2 <num_students> [help_requests_per_student]
 * Example:
 *   ./A2 10 5
 */

#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

enum { CHAIRS = 3 };

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* Counting semaphore: number of students waiting (also wakes TA). */
static sem_t students_sem;
/* TA signals one student at a time to enter for help. */
static sem_t ta_ready_sem;

/* Shared state (protected by mtx) */
static int waiting = 0;
static int total_students = 0;
static int done_students = 0;
static int all_done = 0;

static void die_errno(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        errno = rc;
        die_errno(msg);
    }
}

static int rand_between(unsigned int *seed, int lo_ms, int hi_ms) {
    /* inclusive range [lo_ms, hi_ms] */
    int span = hi_ms - lo_ms + 1;
    return lo_ms + (int)(rand_r(seed) % (unsigned)span);
}

static void msleep(int ms) {
    usleep((useconds_t)ms * 1000u);
}

static void *ta_thread(void *arg) {
    (void)arg;

    for (;;) {
        /* Sleep until at least one student is waiting (or shutdown poke). */
        if (sem_wait(&students_sem) == -1) die_errno("sem_wait(students_sem)");

        die_pthread(pthread_mutex_lock(&mtx), "pthread_mutex_lock");

        /* If everyone is done and nobody is waiting, exit cleanly. */
        if (all_done && waiting == 0) {
            die_pthread(pthread_mutex_unlock(&mtx), "pthread_mutex_unlock");
            break;
        }

        /* Take the next waiting student (if any) and invite them in. */
        if (waiting > 0) {
            waiting--;
            printf("[TA] Calls next student. Waiting now = %d\n", waiting);
            fflush(stdout);

            /* Allow exactly one student to proceed into the office. */
            if (sem_post(&ta_ready_sem) == -1) die_errno("sem_post(ta_ready_sem)");
        }

        die_pthread(pthread_mutex_unlock(&mtx), "pthread_mutex_unlock");

        /* Help takes some time. */
        msleep(200 + (rand() % 401)); /* 200–600ms */
    }

    printf("[TA] No more students. Going home.\n");
    fflush(stdout);
    return NULL;
}

typedef struct {
    int id;
    int requests;
} student_arg_t;

static void *student_thread(void *vp) {
    student_arg_t *a = (student_arg_t *)vp;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(a->id * 2654435761u);

    for (int k = 1; k <= a->requests; k++) {
        /* Programming (random time). */
        msleep(rand_between(&seed, 200, 900));

        /* Try to get help. */
        die_pthread(pthread_mutex_lock(&mtx), "pthread_mutex_lock");

        if (waiting < CHAIRS) {
            waiting++;
            printf("[S%02d] Sits in hallway chair (%d/%d). (Request %d/%d)\n",
                   a->id, waiting, CHAIRS, k, a->requests);
            fflush(stdout);

            /* Notify TA that a student is waiting (wakes TA if sleeping). */
            if (sem_post(&students_sem) == -1) die_errno("sem_post(students_sem)");

            die_pthread(pthread_mutex_unlock(&mtx), "pthread_mutex_unlock");

            /* Wait until TA calls me in. */
            if (sem_wait(&ta_ready_sem) == -1) die_errno("sem_wait(ta_ready_sem)");

            /* Getting help. */
            printf("[S%02d] Getting help from TA...\n", a->id);
            fflush(stdout);
            msleep(rand_between(&seed, 200, 700));
            printf("[S%02d] Done getting help.\n", a->id);
            fflush(stdout);
        } else {
            /* No chair available: leave and try later. */
            printf("[S%02d] Hallway full. Will try later. (Request %d/%d)\n",
                   a->id, k, a->requests);
            fflush(stdout);

            die_pthread(pthread_mutex_unlock(&mtx), "pthread_mutex_unlock");
            /* go back to programming loop */
        }
    }

    /* Mark this student as finished. */
    die_pthread(pthread_mutex_lock(&mtx), "pthread_mutex_lock");
    done_students++;
    if (done_students == total_students) {
        all_done = 1;
        /* Wake TA so it can notice shutdown if it is sleeping. */
        if (sem_post(&students_sem) == -1) die_errno("sem_post(students_sem) shutdown");
    }
    die_pthread(pthread_mutex_unlock(&mtx), "pthread_mutex_unlock");

    printf("[S%02d] Finished all requests. Exiting.\n", a->id);
    fflush(stdout);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_students> [help_requests_per_student]\n", argv[0]);
        return EXIT_FAILURE;
    }

    total_students = atoi(argv[1]);
    if (total_students <= 0) {
        fprintf(stderr, "num_students must be > 0\n");
        return EXIT_FAILURE;
    }

    int requests = 5;
    if (argc >= 3) {
        requests = atoi(argv[2]);
        if (requests <= 0) {
            fprintf(stderr, "help_requests_per_student must be > 0\n");
            return EXIT_FAILURE;
        }
    }

    srand((unsigned)time(NULL));

    if (sem_init(&students_sem, 0, 0) == -1) die_errno("sem_init(students_sem)");
    if (sem_init(&ta_ready_sem, 0, 0) == -1) die_errno("sem_init(ta_ready_sem)");

    pthread_t ta;
    die_pthread(pthread_create(&ta, NULL, ta_thread, NULL), "pthread_create(TA)");

    pthread_t *students = (pthread_t *)calloc((size_t)total_students, sizeof(pthread_t));
    student_arg_t *args = (student_arg_t *)calloc((size_t)total_students, sizeof(student_arg_t));
    if (!students || !args) die_errno("calloc");

    for (int i = 0; i < total_students; i++) {
        args[i].id = i + 1;
        args[i].requests = requests;
        die_pthread(pthread_create(&students[i], NULL, student_thread, &args[i]),
                    "pthread_create(student)");
    }

    for (int i = 0; i < total_students; i++) {
        die_pthread(pthread_join(students[i], NULL), "pthread_join(student)");
    }
    die_pthread(pthread_join(ta, NULL), "pthread_join(TA)");

    free(students);
    free(args);

    if (sem_destroy(&students_sem) == -1) die_errno("sem_destroy(students_sem)");
    if (sem_destroy(&ta_ready_sem) == -1) die_errno("sem_destroy(ta_ready_sem)");
    die_pthread(pthread_mutex_destroy(&mtx), "pthread_mutex_destroy");

    return EXIT_SUCCESS;
}