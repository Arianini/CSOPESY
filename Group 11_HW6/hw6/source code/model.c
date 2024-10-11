#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>  

Process processes[MAX_PROCESSES];
Screen screens[MAX_SCREENS];  // Screen array for screen handling
int process_count = 0;
int current_process = 0;
int screen_count = 0;  // Track number of created screens
HANDLE process_lock;  // Windows mutex
HANDLE stop_event;    // Event to stop the scheduler
int scheduler_running = 0; // 0 means stopped, 1 means running

#define NUM_CORES 4  // We limit this to 4 cores

// Initialize scheduler with processes
void initializeScheduler() {
    scheduler_running = 1;  // Set the scheduler as running
    process_lock = CreateMutex(NULL, FALSE, NULL);
    stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    process_count = 0;
    current_process = 0;

    // Initialize processes
    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process p;
        sprintf(p.name, "process_%d", i + 1);
        p.pid = i + 1;
        p.core_id = -1;  // Initialize core_id to -1
        p.instructions = PRINT_COMMANDS;
        p.finished = 0;
        time_t t = time(NULL);
        struct tm* tm_info = localtime(&t);
        strftime(p.timestamp, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);
        processes[process_count++] = p;
    }

    printf("Scheduler initialized with %d processes.\n", process_count);

    // Start processes across 4 cores in the background
    for (int i = 0; i < NUM_CORES; i++) {
        int* core_id = malloc(sizeof(int));  // Allocate memory for thread argument
        *core_id = i;
        CreateThread(NULL, 0, executeProcess, core_id, 0, NULL);
    }

    
}

// Execute process for each core
DWORD WINAPI executeProcess(LPVOID arg) {
    int core_id = *(int*)arg;
    free(arg);  // Free allocated memory for core_id

    while (1) {
        if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) {
            // Stop the scheduler if the event is set
            return 0;
        }

        WaitForSingleObject(process_lock, INFINITE);  // Lock access to current process

        if (current_process >= process_count) {
            ReleaseMutex(process_lock);
            break;
        }

        Process* process = &processes[current_process];
        process->core_id = core_id;
        current_process++;

        ReleaseMutex(process_lock);  // Unlock after assigning a process

        // Create the file with the process name and core ID
        char filename[50];
        sprintf(filename, "process_%d_core_%d.txt", process->pid, core_id);
        FILE* log_file = fopen(filename, "w");
        if (log_file == NULL) {
            printf("Error creating file %s\n", filename);
            continue;  // Skip this process if file can't be opened
        }

        // Write 100 print commands to the file
        for (int i = 0; i < PRINT_COMMANDS; i++) {
            // Obtain the current timestamp
            time_t t = time(NULL);
            struct tm* tm_info = localtime(&t);
            char buffer[50];
            strftime(buffer, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);

            // Print log entry with the timestamp
            fprintf(log_file, "(%s) Core:%d \"Hello world from %s!\"\n", buffer, core_id, process->name);
            Sleep(100);  // Simulate execution delay
        }

        fclose(log_file);
        process->finished = 1;  // Mark the process as finished
    }
    return 0;
}

// Stop the scheduler by signaling the event
void stopScheduler() {
    SetEvent(stop_event);  // Signal the stop event

    // Mark remaining processes as finished
    WaitForSingleObject(process_lock, INFINITE);
    for (int i = current_process; i < process_count; i++) {
        processes[i].finished = 1;  // Mark as finished
        processes[i].core_id = -1;  // Set core_id to -1
    }
    ReleaseMutex(process_lock);

    scheduler_running = 0;  // Set the scheduler as stopped
    printf("Scheduler stopped.\n");
}

// Function to create a new screen with the given name
void createScreen(char* name) {
    if (screen_count >= MAX_SCREENS) {
        printf("Maximum screen limit reached.\n");
        return;
    }

    // Create a new screen
    strcpy(screens[screen_count].name, name);
    screens[screen_count].id = screen_count + 1;  // Assign a unique ID starting from 1
    screens[screen_count].current_instruction = 0;  // Start from instruction 0
    screens[screen_count].total_instructions = 50;  // Simulating a total of 50 lines of code

    // Get current timestamp
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(screens[screen_count].timestamp, 50, "%m/%d/%Y, %I:%M:%S %p", tm_info);

    printf("Screen '%s' created with ID %d\n", screens[screen_count].name, screens[screen_count].id);
    screen_count++;  // Increment screen count
}

// Function to resume an existing screen by name
void resumeScreen(char* name) {
    int found = 0;  // Flag to check if screen was found
    for (int i = 0; i < screen_count; i++) {
        if (strcmp(screens[i].name, name) == 0) {
            printf("Resuming screen '%s' with ID %d\n", screens[i].name, screens[i].id);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Screen '%s' not found.\n", name);
    }
}
