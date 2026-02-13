part 1:

gcc -Wall -Wextra -pthread PLmutex.c -o PLmutex
./PLmutex 100 50

Part I (PLmutex) — flow (short)

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

gcc -Wall -Wextra -pthread PLsem.c -o PLsem
./PLsem 100