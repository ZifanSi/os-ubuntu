#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// WF-0/Setup: shared balance
static int amount = 0;

// WF-0/Setup: mutex protects amount updates (critical section)
static pthread_mutex_t mtx;

// WF-0/Setup: semDeposit = "deposit slots" (blocks deposit if amount == 400)
static sem_t semDeposit;

// WF-0/Setup: semWithdraw = "withdraw tokens" (blocks withdraw if amount == 0)
static sem_t semWithdraw;

// WF-E: exit on pthread error
static void die_pthread(int rc, const char *msg) {
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(rc));
        exit(EXIT_FAILURE);
    }
}

// WF-E: exit on errno-style error (sem_* returns -1 on failure)
static void die_errno(int rc, const char *msg) {
    if (rc == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *deposit(void *param) {
    // WF-D0: read deposit value (100) from CLI param
    int val = atoi((char *)param);

    // WF-D1: log start (may print even if it blocks next)
    printf("Executing deposit function\n");

    // WF-D2: wait for deposit slot (blocks when semDeposit == 0 => amount at 400)
    sem_wait(&semDeposit);

    // WF-D3: lock mutex to safely update shared amount
    pthread_mutex_lock(&mtx);
    // WF-D4: amount += val
    amount += val;
    // WF-D5: print updated amount
    printf("Amount after deposit = %d\n", amount);
    // WF-D6: unlock mutex
    pthread_mutex_unlock(&mtx);

    // WF-D7: signal withdraw token (now one withdraw can proceed)
    sem_post(&semWithdraw);

    return NULL;
}

void *withdraw(void *param) {
    // WF-W0: read withdraw value (100) from CLI param
    int val = atoi((char *)param);

    // WF-W1: log start (may print even if it blocks next)
    printf("Executing Withdraw function\n");

    // WF-W2: wait for withdraw token (blocks when semWithdraw == 0 => amount at 0)
    sem_wait(&semWithdraw);

    // WF-W3: lock mutex to safely update shared amount
    pthread_mutex_lock(&mtx);
    // WF-W4: amount -= val
    amount -= val;
    // WF-W5: print updated amount
    printf("Amount after Withdrawal = %d\n", amount);
    // WF-W6: unlock mutex
    pthread_mutex_unlock(&mtx);

    // WF-W7: free one deposit slot (space available for a future deposit)
    sem_post(&semDeposit);

    return NULL;
}

int main(int argc, char *argv[]) {
    // WF-0/Setup: read CLI arg (expected 100)
    if (argc != 2) {
        fprintf(stderr, "Usage: %s 100\n", argv[0]);
        return EXIT_FAILURE;
    }

    int val = atoi(argv[1]);
    if (val <= 0) {
        fprintf(stderr, "ERROR: value must be positive (expected 100)\n");
        return EXIT_FAILURE;
    }

    // WF-4: 10 threads total (7 deposits + 3 withdraws)
    pthread_t tids[10];
    pthread_attr_t attr;
    int rc;

    // WF-0/Setup: init mutex
    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    // WF-0/Setup: init thread attributes (default)
    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    // WF-0/Setup: init semaphores
    // semDeposit = 4 slots (0->100->200->300->400)
    die_errno(sem_init(&semDeposit, 0, 4), "sem_init(semDeposit)");
    // semWithdraw = 0 tokens (can't withdraw from 0)
    die_errno(sem_init(&semWithdraw, 0, 0), "sem_init(semWithdraw)");

    // WF-4: create 7 deposit threads
    for (int i = 0; i < 7; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    // WF-4: create 3 withdraw threads
    for (int i = 7; i < 10; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[1]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    // WF-5: join all threads (wait for completion)
    for (int i = 0; i < 10; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    // WF-5: print final amount (should be 400)
    printf("Final amount = %d\n", amount);

    // WF-5: cleanup
    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    die_errno(sem_destroy(&semDeposit), "sem_destroy(semDeposit)");
    die_errno(sem_destroy(&semWithdraw), "sem_destroy(semWithdraw)");

    return 0;
}
