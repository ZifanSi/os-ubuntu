#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int amount = 0;      // shared balance (protected by mutex)
static pthread_mutex_t mtx; // mutex for WF-D1/WF-W1 critical section

void *deposit(void *param) {
    // WF-D0: read deposit value from CLI param
    int val = atoi((char *)param);

    // WF-D1: lock(mutex)
    pthread_mutex_lock(&mtx);
    // WF-D2: amount += deposit
    amount += val;
    // WF-D3: print updated amount
    printf("Deposit amount = %d\n", amount);
    // WF-D4: unlock(mutex)
    pthread_mutex_unlock(&mtx);

    return NULL;
}

void *withdraw(void *param) {
    // WF-W0: read withdraw value from CLI param
    int val = atoi((char *)param);

    // WF-W1: lock(mutex)
    pthread_mutex_lock(&mtx);
    // WF-W2: amount -= withdraw
    amount -= val;
    // WF-W3: print updated amount
    printf("Withdrawal amount = %d\n", amount);
    // WF-W4: unlock(mutex)
    pthread_mutex_unlock(&mtx);

    return NULL;
}

// WF-E: exit on pthread error (keeps code clean)
static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(rc));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    // WF-1: read deposit, withdraw from CLI
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <deposit> <withdraw>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pthread_t tids[6];     // WF-3: 6 threads total
    pthread_attr_t attr;
    int rc;

    // WF-2: init mutex + thread attributes
    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    // WF-3: create 3 withdraw threads
    for (int i = 0; i < 3; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[2]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    // WF-3: create 3 deposit threads
    for (int i = 3; i < 6; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    // WF-4: join all 6 threads (wait for completion)
    for (int i = 0; i < 6; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    // WF-5: print final amount after all updates
    printf("Final amount = %d\n", amount);

    // WF-5: cleanup
    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    return 0;
}
