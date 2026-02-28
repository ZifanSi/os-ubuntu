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

void *ta_thread(void *arg) {
    // TODO: implement TA logic here
    return NULL;
}

int main(void) {
    pthread_t ta;

    if (pthread_create(&ta, NULL, ta_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    if (pthread_join(ta, NULL) != 0) {
        perror("pthread_join");
        return 1;
    }

    return 0;
}