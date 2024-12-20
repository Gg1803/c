#include <stdio.h>
#include <stdlib.h>

#define RAM_SIZE 16
#define VM_SIZE 32
#define PAGE_FRAME_SIZE 2
#define PROCESSES 4
#define PAGES_PER_PROCESS 4
#define TIME_MAX 10000

typedef struct memory
{
    int process_id;
    int page_num;
    int last_accessed;
} memory;

memory* RAM[RAM_SIZE];
memory* VirtualMemory[VM_SIZE];
int page_table[PROCESSES][PAGES_PER_PROCESS];

int timeStep = 0;  // Tracks the simulation time step

// Function to initialize virtual memory and page tables
void initialize_VM()
{
    // Initialize virtual memory
    for (int x = 0; x < VM_SIZE; x += PAGE_FRAME_SIZE)
    {
        int processID = x / 8;      // 8 locations per process (as there are 4 pages per process)
        int pageNum = (x / 2) % 4;

        VirtualMemory[x] = (memory *)malloc(sizeof(memory));
        VirtualMemory[x]->process_id = processID;
        VirtualMemory[x]->page_num = pageNum;
        VirtualMemory[x]->last_accessed = 0; 

        VirtualMemory[x + 1] = VirtualMemory[x];  // Same page occupies two contiguous slots
    }

    // Initialize RAM to NULL (empty)
    for (int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = NULL;
    }

    // Initialize all pages in virtual memory (page_table[i][j] = 99 means page is in virtual memory)
    for (int i = 0; i < PROCESSES; i++) {
        for (int j = 0; j < PAGES_PER_PROCESS; j++) {
            page_table[i][j] = 99;  // All pages start in virtual memory
        }
    }
}

// Find the least recently used page for the process (local LRU)
int find_lru_page(int processID) {
    int min_time = TIME_MAX;
    int lru_index = -1;
    for (int i = 0; i < RAM_SIZE; i += PAGE_FRAME_SIZE) {
        if (RAM[i] && RAM[i]->process_id == processID && RAM[i]->last_accessed < min_time) {
            min_time = RAM[i]->last_accessed;
            lru_index = i;
        }
    }
    return lru_index;
}

// Find the least recently used page globally (global LRU)
int find_global_lru_page() {
    int min_time = TIME_MAX;
    int lru_index = -1;
    for (int i = 0; i < RAM_SIZE; i += PAGE_FRAME_SIZE) {
        if (RAM[i] && RAM[i]->last_accessed < min_time) {
            min_time = RAM[i]->last_accessed;
            lru_index = i;
        }
    }
    return lru_index;
}

// Bring a page from virtual memory to RAM
void load_page_to_RAM(int processID, int page_num) {
    int free_index = -1;

    // Check if there is space in RAM
    for (int i = 0; i < RAM_SIZE; i += PAGE_FRAME_SIZE) {
        if (RAM[i] == NULL) {
            free_index = i;
            break;
        }
    }

    // If no free space, use LRU policy to evict a page
    if (free_index == -1) {
        free_index = find_lru_page(processID);  // Local LRU
        if (free_index == -1) {
            free_index = find_global_lru_page();  // Global LRU if no local pages
        }

        // Evict the page
        int evicted_process_id = RAM[free_index]->process_id;
        int evicted_page_num = RAM[free_index]->page_num;
        page_table[evicted_process_id][evicted_page_num] = 99;  // Mark evicted page as in virtual memory
    }

    // Load the new page into RAM
    for (int i = 0; i < PAGE_FRAME_SIZE; i++) {
        RAM[free_index + i] = VirtualMemory[processID * PAGES_PER_PROCESS + page_num];
        RAM[free_index + i]->last_accessed = timeStep;  // Update last access time
    }

    // Update page table to reflect the new page in RAM
    page_table[processID][page_num] = free_index / PAGE_FRAME_SIZE;
}

// Update last access time for the page
void update_last_access(int processID, int page_num) {
    for (int i = 0; i < RAM_SIZE; i += PAGE_FRAME_SIZE) {
        if (RAM[i] && RAM[i]->process_id == processID && RAM[i]->page_num == page_num) {
            RAM[i]->last_accessed = timeStep;
            break;
        }
    }
}

// Handle page request
void page_request(int pid) {
    int page_num = timeStep % PAGES_PER_PROCESS;  // Get the next page for the process

    // Check if page is already in RAM
    if (page_table[pid][page_num] == 99) {
        // Page is in virtual memory, bring it to RAM
        load_page_to_RAM(pid, page_num);
    }

    // Update last access time
    update_last_access(pid, page_num);

    timeStep++;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    initialize_VM();  // Initialize virtual memory and page tables

    // Open input file for reading process requests
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

    // Read process requests from the input file
    int processID;
    while (fscanf(input_file, "%d", &processID) != EOF) {
        page_request(processID);  // Handle memory access for the process
    }
    fclose(input_file);

    // Open output file for writing results
    FILE *output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return EXIT_FAILURE;
    }

    // Print page tables of each process
    for (int i = 0; i < PROCESSES; i++) {
        for (int j = 0; j < PAGES_PER_PROCESS; j++) {
            fprintf(output_file, "%d", page_table[i][j]);
            if (j < PAGES_PER_PROCESS - 1) {
                fprintf(output_file, ", ");
            }
        }
        fprintf(output_file, "\n");
    }

    // Print the content of the RAM
    for (int i = 0; i < RAM_SIZE; i++) {
        if (RAM[i]) {
            fprintf(output_file, "%d,%d,%d", RAM[i]->process_id, RAM[i]->page_num, RAM[i]->last_accessed);
        } else {
            fprintf(output_file, "NULL");
        }
        if (i % 2 == 1) {
            fprintf(output_file, "; ");
        }
    }
    fprintf(output_file, "\n");

    fclose(output_file);

    return 0;
}
