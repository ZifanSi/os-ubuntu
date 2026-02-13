#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int amount = 0;
static pthread_mutex_t mtx;
static sem_t semDeposit;
static sem_t semWithdraw;

static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(rc));
        exit(EXIT_FAILURE);
    }
}

static void die_errno(int rc, const char *msg) {
    if (rc == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *deposit(void *param) {
    int val = atoi((char *)param);

    printf("Executing deposit function\n");

    sem_wait(&semDeposit);

    pthread_mutex_lock(&mtx);
    amount += val;
    printf("Amount after deposit = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    sem_post(&semWithdraw);

    return NULL;
}

void *withdraw(void *param) {
    int val = atoi((char *)param);

    printf("Executing Withdraw function\n");

    sem_wait(&semWithdraw);

    pthread_mutex_lock(&mtx);
    amount -= val;
    printf("Amount after Withdrawal = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    sem_post(&semDeposit);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s 100\n", argv[0]);
        return EXIT_FAILURE;
    }

    int val = atoi(argv[1]);
    if (val <= 0) {
        fprintf(stderr, "ERROR: value must be positive (expected 100)\n");
        return EXIT_FAILURE;
    }

    pthread_t tids[10];
    pthread_attr_t attr;
    int rc;

    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    die_errno(sem_init(&semDeposit, 0, 4), "sem_init(semDeposit)");
    die_errno(sem_init(&semWithdraw, 0, 0), "sem_init(semWithdraw)");

    for (int i = 0; i < 7; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    for (int i = 7; i < 10; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[1]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    for (int i = 0; i < 10; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    printf("Final amount = %d\n", amount);

    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    die_errno(sem_destroy(&semDeposit), "sem_destroy(semDeposit)");
    die_errno(sem_destroy(&semWithdraw), "sem_destroy(semWithdraw)");

    return 0;
}