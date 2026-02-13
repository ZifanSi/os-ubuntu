part 1:
deposit=100, withdraw=50.

gcc -Wall -Wextra -pthread PLmutex.c -o PLmutex
./PLmutex 100 50


Main

1. Read deposit, withdraw from CLI.

2. Init mutex.

3. Create 3 withdraw threads + 3 deposit threads.

4. join all 6 threads.

5. Print Final amount.

Withdraw thread

1. lock(mutex)

2. amount -= withdraw

3. print amount

4. unlock(mutex)

Deposit thread

1. lock(mutex)

2. amount += deposit

3. print amount

4. unlock(mutex)


part 2:

gcc -Wall -Wextra -pthread PLsem.c -o PLsem
./PLsem 100

0) Setup in main

1. amount = 0

2. Init mtx

3. Init semaphores:

- semDeposit = 4 → 4 deposit “slots” available (0→100→200→300→400)

- semWithdraw = 0 → 0 withdraw tokens (can’t withdraw from 0)

4. Create 7 deposit threads + 3 withdraw threads

5. join all threads → print Final amount


1) Deposit thread 

When a deposit thread runs:

1. Prints "Executing deposit function"

2. sem_wait(&semDeposit)

- If semDeposit > 0, it decrements it and continues

- If semDeposit == 0 (meaning amount is already at 400), it blocks here until a withdraw happens

3. pthread_mutex_lock(&mtx)

- enter critical section (safe access to amount)

4. amount += val and print "Amount after deposit = ..."

5. pthread_mutex_unlock(&mtx)

6. sem_post(&semWithdraw)

- increases withdraw tokens: “now there is money available, one withdraw may proceed”

// deposits are limited by semDeposit so you never go above 400.

2) Withdraw thread workflow

When a withdraw thread runs:

1. Prints "Executing Withdraw function"

2. sem_wait(&semWithdraw)

- If semWithdraw > 0, it decrements it and continues

- If semWithdraw == 0 (meaning amount is 0), it blocks here until a deposit happens

3. pthread_mutex_lock(&mtx)

4. amount -= val and print "Amount after Withdrawal = ..."

5. pthread_mutex_unlock(&mtx)

6. sem_post(&semDeposit)

- increases deposit slots: “space freed, one more deposit may proceed”

// withdrawals are limited by semWithdraw so you never withdraw below 0