// PLsem.c
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Shared state:
  - amount protected by mutex
  - semDeposit controls "space" up to 400
  - semWithdraw controls "funds" available to withdraw
*/
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

/*
  deposit thread:
  - Wait until there is capacity (<400)
  - Then safely add val using mutex
  - Signal that withdrawal is now possible
*/
void *deposit(void *param) {
    int val = atoi((char *)param);

    // (Optional log) Shows thread started; note this prints even if it blocks next
    printf("Executing deposit function\n");

    // BLOCK HERE if amount is already 400 (no capacity tokens)
    sem_wait(&semDeposit);

    // Critical section: update shared amount safely
    pthread_mutex_lock(&mtx);
    amount += val;
    printf("Amount after deposit = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    // After depositing, we have more money -> allow one withdrawal
    sem_post(&semWithdraw);

    return NULL;
}

/*
  withdraw thread:
  - Wait until there are funds (>0)
  - Then safely subtract val using mutex
  - Signal that deposit is now possible (space freed)
*/
void *withdraw(void *param) {
    int val = atoi((char *)param);

    printf("Executing Withdraw function\n");

    // BLOCK HERE if amount is 0 (no funds tokens)
    sem_wait(&semWithdraw);

    pthread_mutex_lock(&mtx);
    amount -= val;
    printf("Amount after Withdrawal = %d\n", amount);
    pthread_mutex_unlock(&mtx);

    // After withdrawing, balance decreased -> allow one more deposit
    sem_post(&semDeposit);

    return NULL;
}

int main(int argc, char *argv[]) {
    /*
      Lab requires 1 CLI arg (e.g., 100)
      It’s used as the deposit/withdraw step amount.
    */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s 100\n", argv[0]);
        return EXIT_FAILURE;
    }

    int val = atoi(argv[1]);
    if (val <= 0) {
        fprintf(stderr, "ERROR: value must be positive (expected 100)\n");
        return EXIT_FAILURE;
    }

    pthread_t tids[10];     // 10 threads total (7 deposit + 3 withdraw)
    pthread_attr_t attr;
    int rc;

    // Initialize mutex
    rc = pthread_mutex_init(&mtx, NULL);
    die_pthread(rc, "pthread_mutex_init");

    // Default thread attributes
    rc = pthread_attr_init(&attr);
    die_pthread(rc, "pthread_attr_init");

    /*
      Initialize semaphores:

      semDeposit starts at 4:
        because you can deposit 4 times (each 100) before reaching 400.

      semWithdraw starts at 0:
        because amount=0 initially, so withdraw should block.
    */
    die_errno(sem_init(&semDeposit, 0, 4), "sem_init(semDeposit)");
    die_errno(sem_init(&semWithdraw, 0, 0), "sem_init(semWithdraw)");

    // Create 7 deposit threads
    for (int i = 0; i < 7; i++) {
        rc = pthread_create(&tids[i], &attr, deposit, argv[1]);
        die_pthread(rc, "pthread_create(deposit)");
    }

    // Create 3 withdraw threads
    for (int i = 7; i < 10; i++) {
        rc = pthread_create(&tids[i], &attr, withdraw, argv[1]);
        die_pthread(rc, "pthread_create(withdraw)");
    }

    // Join all threads to ensure completion
    for (int i = 0; i < 10; i++) {
        rc = pthread_join(tids[i], NULL);
        die_pthread(rc, "pthread_join");
    }

    printf("Final amount = %d\n", amount);

    // Cleanup resources
    rc = pthread_attr_destroy(&attr);
    die_pthread(rc, "pthread_attr_destroy");

    rc = pthread_mutex_destroy(&mtx);
    die_pthread(rc, "pthread_mutex_destroy");

    die_errno(sem_destroy(&semDeposit), "sem_destroy(semDeposit)");
    die_errno(sem_destroy(&semWithdraw), "sem_destroy(semWithdraw)");

    return 0;
}
