#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int amount = 0;
static pthread_mutex_t mtx;

void *deposit(void *param) {
    int val = atoi((char *)param);

    pthread_mutex_lock(&mtx);
    amount += val;
    printf("Deposit amount = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    return NULL;
}

void *withdraw(void *param) {
    int val = atoi((char *)param);

    pthread_mutex_lock(&mtx);
    amount -= val;
    printf("Withdrawal amount = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    return NULL;
}

static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(rc));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <deposit> <withdraw>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pthread_t tids[6];
    pthread_attr_t attr;
    int rc;

    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    for (int i = 0; i < 3; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[2]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    for (int i = 3; i < 6; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    for (int i = 0; i < 6; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    printf("Final amount = %d\n", amount);

    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    return 0;
}