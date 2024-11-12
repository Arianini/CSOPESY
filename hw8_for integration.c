#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_MEMORY 16384
#define MEM_PER_PROC 4096
#define MEM_PER_FRAME 16
#define QUANTUM_CYCLES 4
#define NUM_CPU 2

typedef struct Process {
    int id;
    int memory_required;
    int instructions;
    int current_instruction;
    int memory_start;
} Process;

typedef struct MemoryFrame {
    int occupied;
    int process_id;
} MemoryFrame;

typedef struct Scheduler {
    Process *processes[100];
    int front;
    int rear;
    int count;
} Scheduler;

MemoryFrame memory[MAX_MEMORY / MEM_PER_FRAME];
Scheduler scheduler;
int process_count = 0;

// Utility function to initialize memory
void init_memory() {
    for (int i = 0; i < MAX_MEMORY / MEM_PER_FRAME; i++) {
        memory[i].occupied = 0;
        memory[i].process_id = -1;
    }
}

// Add a process to the scheduler
void add_to_scheduler(Process *process) {
    scheduler.processes[scheduler.rear] = process;
    scheduler.rear = (scheduler.rear + 1) % 100;
    scheduler.count++;
}

// Remove a process from the scheduler
Process* remove_from_scheduler() {
    Process *process = scheduler.processes[scheduler.front];
    scheduler.front = (scheduler.front + 1) % 100;
    scheduler.count--;
    return process;
}

// Memory allocation with first-fit algorithm
int allocate_memory(int process_id) {
    int frames_required = MEM_PER_PROC / MEM_PER_FRAME;
    int start_frame = -1;
    int consecutive_free = 0;

    for (int i = 0; i < MAX_MEMORY / MEM_PER_FRAME; i++) {
        if (memory[i].occupied == 0) {
            if (consecutive_free == 0) start_frame = i;
            consecutive_free++;
            if (consecutive_free == frames_required) {
                for (int j = start_frame; j < start_frame + frames_required; j++) {
                    memory[j].occupied = 1;
                    memory[j].process_id = process_id;
                }
                return start_frame * MEM_PER_FRAME;
            }
        } else {
            consecutive_free = 0;
        }
    }
    return -1;
}

// Release process memory
void release_memory(int process_id) {
    for (int i = 0; i < MAX_MEMORY / MEM_PER_FRAME; i++) {
        if (memory[i].process_id == process_id) {
            memory[i].occupied = 0;
            memory[i].process_id = -1;
        }
    }
}

// Log memory status to a file
void log_memory_status(int quantum_cycle) {
    char filename[50];
    sprintf(filename, "memory_stamp_%02d.txt", quantum_cycle);
    FILE *file = fopen(filename, "w");

    if (!file) return;

    // Get and format current timestamp
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(file, "Timestamp: (%02d/%02d/%d %02d:%02d:%02d)\n",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Count processes in memory
    int processes_in_memory = 0;
    for (int i = 0; i < MAX_MEMORY / MEM_PER_FRAME; i++) {
        if (memory[i].occupied == 1 &&
            (i == 0 || memory[i].process_id != memory[i - 1].process_id)) {
            processes_in_memory++;
        }
    }

    fprintf(file, "Number of processes in memory: %d\n", processes_in_memory);

    // Calculate external fragmentation
    int external_frag = 0;
    for (int i = 0; i < MAX_MEMORY / MEM_PER_FRAME; i++) {
        if (memory[i].occupied == 0) external_frag += MEM_PER_FRAME;
    }
    fprintf(file, "Total external fragmentation in KB: %d\n", external_frag / 1024);

    // Print memory layout as specified
    fprintf(file, "----end----=16384\n");
    for (int i = MAX_MEMORY / MEM_PER_FRAME - 1; i >= 0; i--) {
        if (memory[i].occupied) {
            fprintf(file, "P%d\n%d\n", memory[i].process_id, i * MEM_PER_FRAME);
            while (i > 0 && memory[i].process_id == memory[i - 1].process_id) {
                i--;
            }
        }
    }
    fprintf(file, "=====start----=0\n");
    fclose(file);
}

// Main scheduling loop
void run_scheduler() {
    int quantum_cycle = 0;

    while (quantum_cycle < 15) {
        log_memory_status(quantum_cycle);
        if (scheduler.count == 0) break;

        Process *current_process = remove_from_scheduler();
        current_process->current_instruction += QUANTUM_CYCLES;

        if (current_process->current_instruction >= current_process->instructions) {
            release_memory(current_process->id);
            free(current_process);
        } else {
            add_to_scheduler(current_process);
        }

        quantum_cycle++;
        sleep(1); // simulate time delay per quantum cycle
    }
}

// Test functions for emulation control
void scheduler_test() {
    printf("Scheduler started.\n");
    run_scheduler();
    printf("Scheduler completed.\n");
}

// Main function to initialize and start the emulator
int main() {
    init_memory();
    scheduler.front = scheduler.rear = scheduler.count = 0;

    // Simulate process creation
    for (int i = 0; i < 4; i++) {
        Process *new_process = (Process *)malloc(sizeof(Process));
        new_process->id = i + 1;
        new_process->memory_required = MEM_PER_PROC;
        new_process->instructions = 100;
        new_process->current_instruction = 0;
        int start_address = allocate_memory(new_process->id);

        if (start_address != -1) {
            new_process->memory_start = start_address;
            add_to_scheduler(new_process);
        } else {
            free(new_process);
        }
    }

    scheduler_test();
    return 0;
}
