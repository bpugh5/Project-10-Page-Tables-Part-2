#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Allocate pages for a new process
//
int allocate_page() {
    for (int page_number = 0; page_number < PAGE_COUNT; page_number++) {
        if (mem[page_number] == 0) {
            mem[page_number] = 1;
            return page_number;
        }
    }
    return 0xff;
}

void deallocate_page(int page) {
    mem[page] = 0;
}

void kill_process(int proc_num) {
    int page_table_page = get_page_table(proc_num);

    int page_table = page_table_page * PAGE_SIZE;

    for (int k = 0; k < PAGE_COUNT; k++) {
        if (mem[page_table + k] != 0) {
            deallocate_page(mem[page_table + k]);
        }
    }
    deallocate_page(page_table_page);
}

int get_physical_address(int proc_num, int virtual_addr) {
    int virtual_page = virtual_addr >> 8;
    int offset = virtual_addr & 255;

    int page_table = mem[64 + proc_num];

    int physical_page = mem[page_table * PAGE_SIZE + virtual_page];

    int physical_address = (physical_page << 8) | offset;

    return physical_address;
}

void store_value(int proc_num, int virt_addr, int value) {
    int physical_address = get_physical_address(proc_num, virt_addr);
    mem[physical_address] = value;

    printf("Store proc %d: %d => %d, value=%d\n", proc_num, virt_addr, physical_address, value);
}

void load_value(int proc_num, int virt_addr) {
    int physical_address = get_physical_address(proc_num, virt_addr);
    int value = mem[physical_address];

    printf("Load proc %d: %d => %d, value=%d\n", proc_num, virt_addr, physical_address, value);
}


//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int page_table = allocate_page();

    mem[64 + proc_num] = page_table;

    for (int j = 0; j < page_count; j++) {
        int new_page = allocate_page();

        int pt_addr = get_address(page_table, j);
        mem[pt_addr] = new_page;
    }
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            new_process(atoi(argv[i + 1]), atoi(argv[i + 2]));
        }
        else if (strcmp(argv[i], "kp") == 0) {
            kill_process(atoi(argv[i + 1]));
        }
        else if (strcmp(argv[i], "sb") == 0) {
            store_value(atoi(argv[i + 1]), atoi(argv[i + 2]), atoi(argv[i + 3]));
        }
        else if (strcmp(argv[i], "lb") == 0) {
            load_value(atoi(argv[i + 1]), atoi(argv[i + 2]));
        }
    }
}
