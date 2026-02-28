/*
Compile:
    gcc -pthread A2.c -o A2

Run:
    ./A2
*/

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int waiting_students = 0;

pthread_mutex_t mutex;
sem_t students_sem;
sem_t ta_sem;

void *ta_thread(void *arg) {
    printf("TA is sleeping...\n");
    sem_wait(&students_sem);

    pthread_mutex_lock(&mutex);
    waiting_students--;
    printf("TA wakes up. Waiting students = %d\n", waiting_students);
    pthread_mutex_unlock(&mutex);

    sem_post(&ta_sem);

    printf("TA is helping a student...\n");
    sleep(1);
    printf("TA finished helping a student.\n");

    return NULL;
}

void *student_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    waiting_students++;
    printf("Student arrives. Waiting students = %d\n", waiting_students);
    pthread_mutex_unlock(&mutex);

    sem_post(&students_sem);
    sem_wait(&ta_sem);

    printf("Student is getting help from TA.\n");
    return NULL;
}

int main(void) {
    pthread_t ta;
    pthread_t student;

    pthread_mutex_init(&mutex, NULL);
    sem_init(&students_sem, 0, 0);
    sem_init(&ta_sem, 0, 0);

    if (pthread_create(&ta, NULL, ta_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    sleep(1);

    if (pthread_create(&student, NULL, student_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    pthread_join(student, NULL);
    pthread_join(ta, NULL);

    sem_destroy(&students_sem);
    sem_destroy(&ta_sem);
    pthread_mutex_destroy(&mutex);

    return 0;
}