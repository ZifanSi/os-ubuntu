## HOW TO RUN A1

## Group Members & Team Contribution
- Serbezov, Stanislav — serbezos@mcmasrer.ca
- Si, Zifan — siz@mcmaster.ca

Both members worked together on the coding and testing of seconds.c

## Deliverable
- `seconds.c` — kernel module that creates `/proc/seconds` and reports seconds passed since module load.

## Build & Run (in `newKernel`)
```bash
make clean
make
sudo insmod seconds.ko
cat /proc/seconds
sudo rmmod seconds
```

