part 1:

Main

Read deposit, withdraw from CLI.

Init mutex.

Create 3 withdraw threads + 3 deposit threads.

join all 6 threads.

Print Final amount.

Withdraw thread

lock(mutex)

amount -= withdraw

print amount

unlock(mutex)

Deposit thread

lock(mutex)

amount += deposit

print amount

unlock(mutex)


gcc -Wall -Wextra -pthread PLmutex.c -o PLmutex
./PLmutex 100 50


gcc -Wall -Wextra -pthread PLsem.c -o PLsem
./PLsem 100