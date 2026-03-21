#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 256
#define PAGE_BITS 8
#define OFFSET_MASK 255

#define LOGICAL_SIZE 65536
#define PHYSICAL_SIZE 32768
#define PAGE_COUNT (LOGICAL_SIZE / PAGE_SIZE)
#define FRAME_COUNT (PHYSICAL_SIZE / PAGE_SIZE)
#define TLB_COUNT 16

typedef struct {
    int page;
    int frame;
    int valid;
} TLBItem;

/* Step 1: search page in TLB */
int findInTLB(TLBItem tlb[], int page) {
    int i;
    for (i = 0; i < TLB_COUNT; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            return tlb[i].frame;
        }
    }
    return -1;
}

/* Step 2: add/update TLB entry */
void addToTLB(TLBItem tlb[], int page, int frame, int *tlbPos) {
    tlb[*tlbPos].page = page;
    tlb[*tlbPos].frame = frame;
    tlb[*tlbPos].valid = 1;
    *tlbPos = (*tlbPos + 1) % TLB_COUNT;
}

int replaceTLBEntry(TLBItem tlb[], int oldPage, int newPage, int frame) {
    int i;
    for (i = 0; i < TLB_COUNT; i++) {
        if (tlb[i].valid && tlb[i].page == oldPage) {
            tlb[i].page = newPage;
            tlb[i].frame = frame;
            return 1;
        }
    }
    return 0;
}

int main(void) {
    FILE *addressFile;
    int backingFile;
    signed char *backingData;
    signed char ram[PHYSICAL_SIZE];

    int pageTable[PAGE_COUNT];
    int framePage[FRAME_COUNT];
    TLBItem tlb[TLB_COUNT];

    int nextFrame = 0;
    int nextTLB = 0;
    int faults = 0;
    int hits = 0;
    int total = 0;

    char line[64];
    int i;

    /* Step 3: open files and initialize tables */
    addressFile = fopen("addresses.txt", "r");
    if (addressFile == NULL) {
        perror("addresses.txt");
        return 1;
    }

    backingFile = open("BACKING_STORE.bin", O_RDONLY);
    if (backingFile < 0) {
        perror("BACKING_STORE.bin");
        fclose(addressFile);
        return 1;
    }

    backingData = mmap(NULL, LOGICAL_SIZE, PROT_READ, MAP_PRIVATE, backingFile, 0);
    if (backingData == MAP_FAILED) {
        perror("mmap");
        close(backingFile);
        fclose(addressFile);
        return 1;
    }

    for (i = 0; i < PAGE_COUNT; i++) {
        pageTable[i] = -1;
    }

    for (i = 0; i < FRAME_COUNT; i++) {
        framePage[i] = -1;
    }

    for (i = 0; i < TLB_COUNT; i++) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
        tlb[i].valid = 0;
    }

    /* Step 4: read each logical address and split into page + offset */
    while (fgets(line, sizeof(line), addressFile) != NULL) {
        int logicalAddress;
        int page;
        int offset;
        int frame;
        int physicalAddress;
        int oldPage;
        signed char value;

        logicalAddress = (int)strtol(line, NULL, 10) & 0xFFFF;
        page = logicalAddress >> PAGE_BITS;
        offset = logicalAddress & OFFSET_MASK;

        /* Step 5: check TLB, then page table, then handle page fault if needed */
        frame = findInTLB(tlb, page);

        if (frame != -1) {
            hits++;
        } else {
            frame = pageTable[page];

            if (frame == -1) {
                faults++;

                frame = nextFrame;
                oldPage = framePage[frame];

                if (oldPage != -1) {
                    pageTable[oldPage] = -1;
                }

                memcpy(
                    ram + frame * PAGE_SIZE,
                    backingData + page * PAGE_SIZE,
                    PAGE_SIZE
                );

                pageTable[page] = frame;
                framePage[frame] = page;
                nextFrame = (nextFrame + 1) % FRAME_COUNT;

                if (oldPage != -1) {
                    if (!replaceTLBEntry(tlb, oldPage, page, frame)) {
                        if (findInTLB(tlb, page) == -1) {
                            addToTLB(tlb, page, frame, &nextTLB);
                        }
                    }
                } else {
                    if (findInTLB(tlb, page) == -1) {
                        addToTLB(tlb, page, frame, &nextTLB);
                    }
                }
            } else {
                if (findInTLB(tlb, page) == -1) {
                    addToTLB(tlb, page, frame, &nextTLB);
                }
            }
        }

        /* Step 6: build physical address, print value, and count stats */
        physicalAddress = frame * PAGE_SIZE + offset;
        value = ram[physicalAddress];

        printf("Virtual address: %d Physical address = %d Value=%d\n",
               logicalAddress, physicalAddress, value);

        total++;
    }

    /* Step 7: print final results and clean up */
    printf("Total addresses = %d\n", total);
    printf("Page_faults = %d\n", faults);
    printf("TLB Hits = %d\n", hits);

    munmap(backingData, LOGICAL_SIZE);
    close(backingFile);
    fclose(addressFile);

    return 0;
}