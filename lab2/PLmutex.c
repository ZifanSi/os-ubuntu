// PLmutex.c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Shared data (global):
  - amount: the shared "bank balance"
  - mtx: mutex that protects amount
*/
static int amount = 0;
static pthread_mutex_t mtx;

/*
  deposit thread function:
  - Convert the thread parameter to an integer deposit value
  - Lock mutex -> safely update shared variable -> print -> unlock
*/
void *deposit(void *param) {
    int val = atoi((char *)param);

    // BEGIN critical section (protect shared variable)
    pthread_mutex_lock(&mtx);

    amount += val;
    printf("Deposit amount = %d\n", amount);

    // END critical section
    pthread_mutex_unlock(&mtx);

    return NULL;
}

/*
  withdraw thread function:
  - Convert parameter to withdraw value
  - Lock mutex -> safely update shared variable -> print -> unlock
*/
void *withdraw(void *param) {
    int val = atoi((char *)param);

    // BEGIN critical section
    pthread_mutex_lock(&mtx);

    amount -= val;
    printf("Withdrawal amount = %d\n", amount);

    // END critical section
    pthread_mutex_unlock(&mtx);

    return NULL;
}

/*
  Helper to fail fast on pthread API errors (pthread functions return error codes)
*/
static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(rc));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    /*
      Lab requires 2 CLI args:
      argv[1] = deposit amount
      argv[2] = withdraw amount
    */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <deposit> <withdraw>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pthread_t tids[6];      // store the 6 thread IDs so we can join them later
    pthread_attr_t attr;    // optional thread attributes object
    int rc;

    // Initialize mutex (must happen before lock/unlock)
    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    // Initialize thread attributes (default attributes)
    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    /*
      Create 3 withdraw threads first (order doesn’t matter for correctness,
      but it affects the printed ordering you see).
    */
    for (int i = 0; i < 3; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[2]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    // Create 3 deposit threads
    for (int i = 3; i < 6; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    /*
      IMPORTANT:
      Join all threads so main waits until they are done.
      Otherwise main could exit early and kill the process.
    */
    for (int i = 0; i < 6; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    // Now it’s safe to print final result (all updates completed)
    printf("Final amount = %d\n", amount);

    // Cleanup
    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    return 0;
}
