#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK (PAGE_SIZE - 1)
#define PAGE_MASK 0xFF

#define LOGICAL_MEMORY_SIZE (1 << 16)
#define PHYSICAL_MEMORY_SIZE (1 << 15)
#define NUM_PAGES (LOGICAL_MEMORY_SIZE / PAGE_SIZE)
#define NUM_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)
#define TLB_SIZE 16

typedef struct {
    int page_number;
    int frame_number;
    int valid;
} TLBEntry;

int search_TLB(const TLBEntry tlb[], int page_number) {
    int i;
    for (i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].valid && tlb[i].page_number == page_number) {
            return tlb[i].frame_number;
        }
    }
    return -1;
}

void TLB_Add(TLBEntry tlb[], int page_number, int frame_number, int *next_insert) {
    tlb[*next_insert].page_number = page_number;
    tlb[*next_insert].frame_number = frame_number;
    tlb[*next_insert].valid = 1;
    *next_insert = (*next_insert + 1) % TLB_SIZE;
}

int TLB_Update(TLBEntry tlb[], int replaced_page, int new_page, int frame_number) {
    int i;
    for (i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].valid && tlb[i].page_number == replaced_page) {
            tlb[i].page_number = new_page;
            tlb[i].frame_number = frame_number;
            return 1;
        }
    }
    return 0;
}

int main(void) {
    FILE *addr_file;
    int backing_fd;
    signed char *backing_store;
    signed char physical_memory[PHYSICAL_MEMORY_SIZE];
    int page_table[NUM_PAGES];
    int frame_to_page[NUM_FRAMES];
    TLBEntry tlb[TLB_SIZE];
    int next_frame = 0;
    int next_tlb_insert = 0;
    int page_faults = 0;
    int tlb_hits = 0;
    int total_addresses = 0;
    char buffer[64];
    int i;

    addr_file = fopen("addresses.txt", "r");
    if (addr_file == NULL) {
        perror("fopen addresses.txt");
        return 1;
    }

    backing_fd = open("BACKING_STORE.bin", O_RDONLY);
    if (backing_fd < 0) {
        perror("open BACKING_STORE.bin");
        fclose(addr_file);
        return 1;
    }

    backing_store = mmap(NULL, LOGICAL_MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);
    if (backing_store == MAP_FAILED) {
        perror("mmap BACKING_STORE.bin");
        close(backing_fd);
        fclose(addr_file);
        return 1;
    }

    for (i = 0; i < NUM_PAGES; ++i) {
        page_table[i] = -1;
    }
    for (i = 0; i < NUM_FRAMES; ++i) {
        frame_to_page[i] = -1;
    }
    for (i = 0; i < TLB_SIZE; ++i) {
        tlb[i].valid = 0;
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
    }

    while (fgets(buffer, sizeof(buffer), addr_file) != NULL) {
        int logical_address = (int)strtol(buffer, NULL, 10) & 0xFFFF;
        int page_number = (logical_address >> OFFSET_BITS) & PAGE_MASK;
        int offset = logical_address & OFFSET_MASK;
        int frame_number = search_TLB(tlb, page_number);
        int physical_address;
        signed char value;

        if (frame_number != -1) {
            tlb_hits++;
        } else {
            frame_number = page_table[page_number];

            if (frame_number == -1) {
                int replaced_page;

                page_faults++;
                frame_number = next_frame;
                replaced_page = frame_to_page[frame_number];

                if (replaced_page != -1) {
                    page_table[replaced_page] = -1;
                }

                memcpy(
                    physical_memory + (frame_number * PAGE_SIZE),
                    backing_store + (page_number * PAGE_SIZE),
                    PAGE_SIZE
                );

                page_table[page_number] = frame_number;
                frame_to_page[frame_number] = page_number;
                next_frame = (next_frame + 1) % NUM_FRAMES;

                if (replaced_page != -1) {
                    if (!TLB_Update(tlb, replaced_page, page_number, frame_number)) {
                        TLB_Add(tlb, page_number, frame_number, &next_tlb_insert);
                    }
                } else {
                    TLB_Add(tlb, page_number, frame_number, &next_tlb_insert);
                }
            }
        }

        physical_address = (frame_number << OFFSET_BITS) | offset;
        value = physical_memory[physical_address];

        printf(
            "Virtual address: %d Physical address = %d Value=%d \n",
            logical_address,
            physical_address,
            value
        );

        total_addresses++;
    }

    printf("Total addresses = %d \n", total_addresses);
    printf("Page_faults = %d \n", page_faults);
    printf("TLB Hits = %d \n", tlb_hits);

    munmap(backing_store, LOGICAL_MEMORY_SIZE);
    close(backing_fd);
    fclose(addr_file);
    return 0;
}
