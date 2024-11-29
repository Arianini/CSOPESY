#include "model.h"
#include "view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>
#include <direct.h> 
extern int in_main_menu;

// Configuration variables
int NUM_CORES;
int MAX_PROCESSES;
char config_schedulerType[20];
int config_quantumCycles;
float config_batchProcessFreq;
int config_minIns;
int config_maxIns;
float config_delayPerExec;
// new parameters for OS emulator memory manager:
int config_MAX_OMEM;
int config_mem_perFrame;
int config_min_mem_per_proc;
int config_max_mem_per_proc;

MemoryBlock* memory = NULL; // Allocate based on config_MAX_OMEM / config_mem_perFrame
int free_memory;

Process* processes = NULL;
Process* current_screen_process = NULL;
Screen *screens = NULL;
HANDLE process_lock;
HANDLE stop_event;
HANDLE monitor_thread;
HANDLE* coreThreads = NULL;  // Dynamic array for core threads
int process_count = 0;
int current_process = 0;
int screen_count = 0;
int active_cores = 0; 
int scheduler_running = 0;
int stop_generating = 0; 
int global_current_process_index = 0;
int in_scheduler_test = 0; // Flag to suppress print statements during scheduler test
static int rr_index = 0;
bool exit_monitor_thread = false;

//MCO2 global variables
int total_cpu_ticks = 0;
int active_cpu_ticks = 0;
int idle_cpu_ticks = 0;
int pages_paged_in = 0;
int pages_paged_out = 0;

int initializeSchedulerFromFile() {
    FILE* config = fopen("config.txt", "r");
    if (!config) {
        setConsoleColor(12);
        printf("Error: 'config.txt' not found.\n");
        resetConsoleColor();
        return 0;
    }
    if (fscanf(config, "%d %s %d %f %d %d %f %d %d %d %d",
               &NUM_CORES,
               config_schedulerType,
               &config_quantumCycles,
               &config_batchProcessFreq,
               &config_minIns,
               &config_maxIns,
               &config_delayPerExec,
               &config_MAX_OMEM,
               &config_mem_perFrame,
               &config_min_mem_per_proc,
               &config_max_mem_per_proc) != 11) {
        setConsoleColor(12);
        printf("Error: Invalid format in config.txt.\n");
        resetConsoleColor();
        fclose(config);
        return 0;
    }
    fclose(config);

    MAX_PROCESSES = 10000;  // Allocate space for N processes

    // Allocate memory for the processes array
    processes = (Process*)malloc(sizeof(Process) * MAX_PROCESSES);
    if (!processes) {
        printf("Error: Initial memory allocation for processes failed.\n");
        return 0;
    }

    screens = (Screen*)malloc(sizeof(Screen) * MAX_PROCESSES); 
    if (!screens) {
        printf("Error: Initial memory allocation for screens failed.\n");
        free(processes);  // Free processes if screens allocation fails
        return 0;
    }

    process_lock = CreateMutex(NULL, FALSE, NULL);
    stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    scheduler_running = 1;

    // Dynamically allocate memory for core threads based on NUM_CORES
    coreThreads = (HANDLE*)malloc(sizeof(HANDLE) * NUM_CORES);
    if (!coreThreads) {
        printf("Error: Could not allocate memory for core threads.\n");
        free(processes);
        free(screens);
        return 0;
    }

    // Create a dedicated thread for each core
    for (int i = 0; i < NUM_CORES; i++) {
        int* core_id = malloc(sizeof(int)); // Allocate memory to pass core_id to the thread
        *core_id = i;
        coreThreads[i] = CreateThread(NULL, 0, executeProcess, core_id, 0, NULL);
        if (coreThreads[i] == NULL) {
            printf("Error: Could not create thread for core %d.\n", i);
            free(core_id); // Free the memory if thread creation fails
        }
    }

    free_memory = config_MAX_OMEM;
    memory = malloc((config_MAX_OMEM / config_mem_perFrame) * sizeof(MemoryBlock));

    // Initialize memory array dynamically
    for (int i = 0; i < config_MAX_OMEM / config_mem_perFrame; i++) {
        memory[i].pid = -1;
    }
    
    return 1;
}

void runSchedulerTest() {
    printf("Running scheduler test...\n");
    in_scheduler_test = 1;
    scheduler_running = 1;
    stop_generating = 0;  // Reset flag to allow process generation

    ResetEvent(stop_event);

    // Start or resume the process generation thread
    HANDLE processGenerationThread = CreateThread(NULL, 0, generateProcesses, NULL, 0, NULL);
    if (processGenerationThread == NULL) {
        printf("Error: Could not create process generation thread.\n");
        return;
    }

    printf("Scheduler test started. You can enter commands while processes are being generated.\n");

    // Close the handle to avoid resource leaks
    CloseHandle(processGenerationThread);

    in_scheduler_test = 0;
}

void resizeProcessesArray() {

    int new_size = MAX_PROCESSES * 2;
    Process* temp = (Process*)realloc(processes, sizeof(Process) * new_size);

    if (temp) {
        // Reassign only if realloc succeeds
        processes = temp;
        MAX_PROCESSES = new_size;
    } else {
        printf("Error: Memory reallocation failed.\n");
        // Exit or handle the error as needed
    }
}

// Monitor thread function to resize memory in the background
DWORD WINAPI memoryMonitor(LPVOID lpParam) {
    while (!exit_monitor_thread) {
        if ((float)process_count / MAX_PROCESSES >= RESIZE_THRESHOLD) {
            resizeProcessesArray();
        }
        Sleep(1000);  // Check memory every second
    }
    return 0;
}

// Initialize scheduler with proactive memory management
void initializeSchedulerWithProactiveMemory() {
    processes = (Process*)malloc(sizeof(Process) * MAX_PROCESSES);
    if (!processes) {
        printf("Error: Initial memory allocation failed.\n");
        exit(1);
    }

    // Start the background memory monitor thread
    monitor_thread = CreateThread(NULL, 0, memoryMonitor, NULL, 0, NULL);
}

void stopScheduler() {
    if (!scheduler_running) {
        printf("Scheduler is already stopped.\n");
        return;
    }

    printf("Stopping process generation in scheduler-test...\n");

    stop_generating = 1;  // Set flag to stop generating new processes
}

DWORD WINAPI executeProcess(LPVOID arg) {
    int core_id = *(int*)arg;
    free(arg);

    while (scheduler_running) {
        if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) {
            return 0;
        }

        if (strcmp(config_schedulerType, "rr") == 0) {
            roundRobinScheduler(core_id);
        } else if (strcmp(config_schedulerType, "fcfs") == 0) {
            fcfsScheduler(core_id);
        } else {
            printf("Unknown scheduler type: %s\n", config_schedulerType);
        }

        Sleep(config_delayPerExec * 100);  // Delay to prevent CPU overuse
    }

    return 0;
}

DWORD WINAPI generateProcesses(LPVOID lpParam) {
    int scheduler_process_count = 0;

    while (scheduler_running && !stop_generating) {
        for (int i = 0; i < 1; i++) {
            if (stop_generating) break;

            if (process_count >= MAX_PROCESSES) {
                resizeProcessesArray();
            }

            WaitForSingleObject(process_lock, INFINITE);

            Process* new_process = &processes[process_count++];
            snprintf(new_process->name, sizeof(new_process->name), "Process%02d", scheduler_process_count + 1);
            new_process->pid = process_count;
            new_process->core_id = -1;
            new_process->instructions = config_minIns + (rand() % (config_maxIns - config_minIns + 1));
            new_process->instructions_completed = 0;
            new_process->finished = 0;

            // Allocate memory for this process
            new_process->memory_size = config_min_mem_per_proc + 
                                       (rand() % (config_max_mem_per_proc - config_min_mem_per_proc + 1));
            new_process->allocated_memory = allocateMemory(new_process->pid, new_process->memory_size);

            scheduler_process_count++;
            ReleaseMutex(process_lock);

            // Retry memory allocation for existing processes
            retryMemoryAllocation();

            Sleep((int)(config_delayPerExec * 1000)); // Delay for next process
        }

        Sleep((int)(config_batchProcessFreq * 1000));
    }

    return 0;
}

void roundRobinScheduler(int core_id) {
    int quantum = config_quantumCycles;
    static int rr_index = 0; // Retain the last index for round-robin selection
    bool all_finished = false;

    while (scheduler_running && !all_finished) {
        all_finished = true;

        WaitForSingleObject(process_lock, INFINITE);

        // Retry memory allocation for failed processes
        retryMemoryAllocation();

        // Select the next process in the round-robin queue
        for (int i = 0; i < process_count; i++) {
            int idx = (rr_index + i) % process_count;

            if (!processes[idx].finished && processes[idx].core_id == -1 &&
                processes[idx].allocated_memory != -1) {
                // Only schedule processes with valid memory allocation
                processes[idx].core_id = core_id;
                rr_index = (idx + 1) % process_count; // Update round-robin index
                all_finished = false; // At least one process can run

                ReleaseMutex(process_lock);

                int time_slice = quantum;
                while (time_slice > 0 && processes[idx].instructions_completed < processes[idx].instructions) {
                    Sleep(config_delayPerExec * 100);
                    processes[idx].instructions_completed++;
                    time_slice--;
                }

                WaitForSingleObject(process_lock, INFINITE);
                if (processes[idx].instructions_completed >= processes[idx].instructions) {
                    processes[idx].finished = 1;
                    freeMemory(processes[idx].pid); // Free memory after process finishes
                }
                processes[idx].core_id = -1; // Release core
                ReleaseMutex(process_lock);
                break;
            }
        }

        ReleaseMutex(process_lock);
    }
}

void fcfsScheduler(int core_id) {
    while (scheduler_running) {
        Process* current_process = NULL;

        // Acquire lock to safely access shared resources
        WaitForSingleObject(process_lock, INFINITE);

        // Find the next available process that is not yet finished, not assigned to any core, and has memory allocated
        for (int i = global_current_process_index; i < process_count; i++) {
            if (!processes[i].finished && processes[i].core_id == -1) {
                // Check if the process has memory allocated or if memory can be allocated
                if (processes[i].allocated_memory == -1) {
                    processes[i].allocated_memory = allocateMemory(processes[i].pid, processes[i].memory_size);
                }

                // Only assign the process if memory allocation is successful
                if (processes[i].allocated_memory != -1) {
                    current_process = &processes[i];
                    current_process->core_id = core_id; // Assign to this core
                    global_current_process_index = i + 1; // Move to next unprocessed index
                    break;
                }
            }
        }

        ReleaseMutex(process_lock);

        // If no process is available to run, wait and check again
        if (current_process == NULL) {
            Sleep(config_delayPerExec * 100);
            continue;
        }

        // Execute the process completely
        while (current_process->instructions_completed < current_process->instructions && scheduler_running) {
            Sleep(config_delayPerExec * 100); // Simulate execution delay
            current_process->instructions_completed++;
        }

        // Mark as finished and release the core assignment
        WaitForSingleObject(process_lock, INFINITE);
        current_process->finished = 1;
        current_process->last_core_id = core_id; // Store the core where it finished
        current_process->core_id = -1; // Release core assignment

        // Free the memory used by the process after it finishes
        freeMemory(current_process->pid);

        ReleaseMutex(process_lock);
    }
}

void reportProgress() {
    printf("+-----------------------------------------------+\n");

    int cores_in_use = 0;
    WaitForSingleObject(process_lock, INFINITE);
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break;
            }
        }
    }
    ReleaseMutex(process_lock);

    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100;
    printf("CPU utilization: %.0f%%\n", cpu_utilization);
    printf("Cores used: %d\n", cores_in_use);
    printf("Cores available: %d\n", NUM_CORES - cores_in_use);
    printf("+-----------------------------------------------+\n");

    // Display running processes
    printf("\033[1;36mRunning processes:\033[0m\n");
    int running_found = 0;
    WaitForSingleObject(process_lock, INFINITE);
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            printf("%s (PID: %d) Core: %d  %d / %d\n", 
                   processes[i].name, processes[i].pid, processes[i].core_id, 
                   processes[i].instructions_completed, processes[i].instructions);
            running_found = 1;
        }
    }
    ReleaseMutex(process_lock);
    if (!running_found) {
        printf("No running processes.\n");
    }

    // Display finished processes
    printf("\033[1;36mFinished processes:\033[0m\n");
    int finished_found = 0;
    WaitForSingleObject(process_lock, INFINITE);
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            printf("%s (PID: %d) Finished  %d / %d\n",
                   processes[i].name, processes[i].pid,
                   processes[i].instructions_completed, processes[i].instructions);
            finished_found = 1;
        }
    }
    ReleaseMutex(process_lock);
    if (!finished_found) {
        printf("No finished processes.\n");
    }

    printf("+-----------------------------------------------+\n");
}

DWORD WINAPI executeScreenProcess(LPVOID arg) {
    Screen *screen = (Screen *)arg;
    
    while (screen->current_instruction < screen->total_instructions) {
        // Simulate execution by incrementing the current instruction
        screen->current_instruction += (rand() % 100) + 1;  // Increment randomly for variability
        
        // Ensure it does not exceed the total instructions
        if (screen->current_instruction > screen->total_instructions) {
            screen->current_instruction = screen->total_instructions;
        }

        // Sleep to simulate delay per execution cycle
        Sleep(config_delayPerExec * 100);  // Multiply by 1000 to convert to milliseconds
    }
    
    return 0;
}

// Check for an available core; return -1 if none is available
int getAvailableCore() {
    WaitForSingleObject(process_lock, INFINITE);  // Lock to safely check core availability
    for (int i = 0; i < NUM_CORES; i++) {
        bool core_in_use = false;
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                core_in_use = true;
                break;
            }
        }
        // Ensure no thread is currently running on this core
        if (!core_in_use && coreThreads[i] == NULL) {
            ReleaseMutex(process_lock);
            return i;
        }
    }
    ReleaseMutex(process_lock);
    return -1;  // No available core
}

// Start the new process based on the scheduler type
DWORD WINAPI startManualProcess(LPVOID arg) {
    Process* process = (Process*)arg;
    int core_id = getAvailableCore();

    while (!process->finished) {
        if (core_id != -1) {
            // Assign core if available and start execution based on scheduler type
            process->core_id = core_id;

            if (strcmp(config_schedulerType, "rr") == 0) {
                int time_slice = config_quantumCycles;
                while (time_slice > 0 && process->instructions_completed < process->instructions) {
                    Sleep(config_delayPerExec * 100);
                    process->instructions_completed++;
                    time_slice--;
                }
                // Check if the process is finished after the time slice
                if (process->instructions_completed >= process->instructions) {
                    process->finished = 1;
                    process->last_core_id = core_id;
                }
            } else if (strcmp(config_schedulerType, "fcfs") == 0) {
                while (process->instructions_completed < process->instructions) {
                    Sleep(config_delayPerExec * 100);
                    process->instructions_completed++;
                }
                process->finished = 1;
                process->last_core_id = core_id;
            }

            // Release core after completion or time slice
            if (process->finished) {
                process->core_id = -1;
            }
            core_id = getAvailableCore();  // Check if another core is free for subsequent cycles
        } else {
            // Wait until a core is free
            Sleep(500);
            core_id = getAvailableCore();
        }
    }
    return 0;
}

Process* getProcessByName(char* name) {
    for (int i = 0; i < process_count; i++) {
        if (strcmp(processes[i].name, name) == 0) {
            return &processes[i];
        }
    }
    return NULL;  // Return NULL if the process is not found
}

// Creates a new process and assigns it based on scheduling configuration
Process* createNewProcess(char* name) {
    if (process_count >= MAX_PROCESSES) {
        resizeProcessesArray();
    }
    if (process_count >= MAX_PROCESSES) {
        printf("Error: Process limit reached. Could not create new process '%s'.\n", name);
        return NULL;
    }

    Process* new_process = &processes[process_count++];
    snprintf(new_process->name, sizeof(new_process->name), "%s", name);
    new_process->pid = process_count;
    new_process->core_id = -1;
    new_process->instructions = config_minIns + (rand() % (config_maxIns - config_minIns + 1));
    new_process->instructions_completed = 0;
    new_process->finished = 0;

    // Setting the timestamp
    time_t t = time(NULL);
    strftime(new_process->timestamp, sizeof(new_process->timestamp), "%m/%d/%Y %I:%M:%S %p", localtime(&t));

    printf("New process '%s' created with PID %d.\n", new_process->name, new_process->pid);
    return new_process;
}

void createScreen(char* process_name) {
    WaitForSingleObject(process_lock, INFINITE); // Lock for thread safety

    // Check if the screen count exceeds the maximum allowed screens
    if (screen_count >= MAX_SCREENS) {
        printf("Maximum screen limit reached.\n");
        ReleaseMutex(process_lock);
        return;
    }

    // Attempt to retrieve the process by name
    Process* process = getProcessByName(process_name);
    if (!process) {
        printf("Process '%s' not found. Creating a new process with this name.\n", process_name);
        
        // Create a new process if it doesn't exist
        process = createNewProcess(process_name);
        if (!process) {
            printf("Error: Could not create a new process with name '%s'.\n", process_name);
            ReleaseMutex(process_lock);
            return;
        }
    }

    // Initialize a new screen for the found or created process
    Screen new_screen;
    strcpy(new_screen.name, process_name);
    new_screen.id = screen_count + 1;
    new_screen.current_instruction = process->instructions_completed;
    new_screen.total_instructions = process->instructions;
    new_screen.created_by_screen_s = true;
    
    // Set the current timestamp for the screen
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(new_screen.timestamp, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);

    // Add the screen to the screens array
    screens[screen_count++] = new_screen;

    // Release the lock after safely modifying shared resources
    ReleaseMutex(process_lock);

    // Switch to this screen and display process details
    in_main_menu = 0;
    displayScreen(&new_screen);  // Ensure displayScreen is properly set to output screen information
}

Screen* getScreenByName(char* name) {
    for (int i = 0; i < screen_count; i++) {
        if (strcmp(screens[i].name, name) == 0) {
            return &screens[i];
        }
    }
    return NULL;
}

void resumeScreen(char* name) {
    int found = 0;

    for (int i = 0; i < screen_count; ++i) {
        // Find the screen with the specified name that was created by `screen -s`
        if (strcmp(screens[i].name, name) == 0 && screens[i].created_by_screen_s) {
            Process* process = getProcessByName(name);  // Retrieve the process by name

            if (!process) {
                printf("Error: Process '%s' not found.\n", name);
                return;
            }

            found = 1;
            in_main_menu = 0;
            current_screen_process = process; // Set current process for screen commands

            // Lock access to the process data to get the latest state
            WaitForSingleObject(process_lock, INFINITE);

            // Display the current snapshot of process status at resume time
            printf("Resuming screen for process: %s\n", name);
            printf("Process: %s\n", process->name);
            printf("ID: %d\n", process->pid);
            printf("Current instruction line (at resume): %d/%d\n", 
                   process->instructions_completed, process->instructions);

            if (process->instructions_completed >= process->instructions) {
                printf("Status: Finished!\n");
            }

            // Release the lock after getting the snapshot
            ReleaseMutex(process_lock);

            // Allow `process-smi` and `exit` commands
            return;
        }
    }

    if (!found) {
        printf("Screen '%s' not found or was not created by 'screen -s'.\n", name);
    }
}

void generateUtilizationReport() {
    printf("Generating CPU utilization report...\n");

    FILE* reportFile = fopen("csopesy-log.txt", "w");
    if (!reportFile) {
        printf("Error: Could not create or open 'csopsey-log.txt'.\n");
        return;
    }

    fprintf(reportFile, "+-----------------------------------------------+\n");

    int cores_in_use = 0;
    WaitForSingleObject(process_lock, INFINITE);
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break;
            }
        }
    }
    ReleaseMutex(process_lock);

    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100;
    fprintf(reportFile, "CPU utilization: %.0f%%\n", cpu_utilization);
    fprintf(reportFile, "Cores used: %d\n", cores_in_use);
    fprintf(reportFile, "Cores available: %d\n", NUM_CORES - cores_in_use);
    fprintf(reportFile, "+-----------------------------------------------+\n");

    // Running Processes
    fprintf(reportFile, "Running processes:\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            fprintf(reportFile, "%s (PID: %d) Core: %d  %d / %d\n", 
                    processes[i].name, processes[i].pid, processes[i].core_id, 
                    processes[i].instructions_completed, processes[i].instructions);
            running_found = 1;
        }
    }
    if (!running_found) {
        fprintf(reportFile, "No running processes.\n");
    }

    // Finished Processes
    fprintf(reportFile, "Finished processes:\n");
    int finished_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            fprintf(reportFile, "%s (PID: %d) Finished  %d / %d\n",
                    processes[i].name, processes[i].pid,
                    processes[i].instructions_completed, processes[i].instructions);
            finished_found = 1;
        }
    }
    if (!finished_found) {
        fprintf(reportFile, "No finished processes.\n");
    }

    fprintf(reportFile, "+-----------------------------------------------+\n");

    fclose(reportFile);
    printf("Utilization report generated in 'csopsey-log.txt'.\n");
}

//NEW WEEK 8 START
void initializeMemory() {
    for (int i = 0; i < config_MAX_OMEM / config_mem_perFrame; i++) {
        memory[i].start_addr = i * config_mem_perFrame;
        memory[i].size = config_mem_perFrame;
        memory[i].pid = -1;
    }
}
int allocateMemory(int pid, int required_size) {
    if (required_size > config_MAX_OMEM) {
        //printf("Error: Process %d requires more memory (%d KB) than available (%d KB).\n", 
               //pid, required_size, config_MAX_OMEM);
        return -1; // Indicate failed memory allocation
    }

    int frames_needed = (required_size + config_mem_perFrame - 1) / config_mem_perFrame;
    for (int i = 0; i <= (config_MAX_OMEM / config_mem_perFrame) - frames_needed; i++) {
        bool enough_space = true;
        for (int j = 0; j < frames_needed; j++) {
            if (memory[i + j].pid != -1) {
                enough_space = false;
                i += j; // Skip ahead
                break;
            }
        }

        if (enough_space) {
            for (int k = 0; k < frames_needed; k++) {
                memory[i + k].pid = pid;
            }
            free_memory -= frames_needed * config_mem_perFrame;
            pages_paged_in += frames_needed;
            return i * config_mem_perFrame; // Return start address
        }
    }
    
    pages_paged_out++;
    return -1; // Allocation failed
}

void retryMemoryAllocation() {
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].allocated_memory == -1) {
            processes[i].allocated_memory = allocateMemory(processes[i].pid, processes[i].memory_size);
            
        }
    }
}

void freeMemory(int pid) {
    for (int i = 0; i < config_MAX_OMEM / config_mem_perFrame; i++) {
        if (memory[i].pid == pid) {
            memory[i].pid = -1;
            free_memory += config_mem_perFrame;
            pages_paged_out++;
        }
    }
}

void recordMemorySnapshot(int cycle) {
    time_t rawtime;
    struct tm *timeinfo;
    char formattedTime[80];

    // Get current time for the timestamp
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(formattedTime, sizeof(formattedTime), "(%m/%d/%Y %I:%M:%S%p)", timeinfo);

    // Define folder for memory snapshots
    const char *folderPath = "./memory_snapshots/";
    _mkdir(folderPath); // Create the folder if it doesn't exist (Windows-specific)

    // Construct the full file path
    char filename[150];
    sprintf(filename, "%smemory_stamp_%d.txt", folderPath, cycle);

    // Open the file for writing
    FILE *snapshotFile = fopen(filename, "w");
    if (!snapshotFile) {
        printf("Error: Unable to create memory snapshot file '%s'.\n", filename);
        return;
    }

    // Write the timestamp and initial info to the file
    fprintf(snapshotFile, "Timestamp: %s\n", formattedTime);

    // Count the number of unique processes in memory
    int processCount = 0;
    int last_pid = -1;
    int external_fragmentation = 0;
    int contiguous_free_space = 0;
    bool in_free_block = false;

    for (int i = 0; i < (config_MAX_OMEM / config_mem_perFrame); i++) {
        if (memory[i].pid != -1) {
            if (in_free_block) {
                external_fragmentation += contiguous_free_space;
                contiguous_free_space = 0;
                in_free_block = false;
            }
            if (memory[i].pid != last_pid) {
                processCount++;
                last_pid = memory[i].pid;
            }
        } else {
            if (!in_free_block) {
                in_free_block = true;
            }
            contiguous_free_space += config_mem_perFrame;
        }
    }

    // Add the last contiguous free space block if it exists
    if (in_free_block) {
        external_fragmentation += contiguous_free_space;
    }

    // Write the count of processes in memory and external fragmentation in KB
    fprintf(snapshotFile, "Number of processes in memory: %d\n", processCount);
    fprintf(snapshotFile, "Total external fragmentation in KB: %d\n", external_fragmentation / 1024);

    // Print memory boundaries and layout
    fprintf(snapshotFile, "====end==== = %d\n", config_MAX_OMEM);
    for (int i = (config_MAX_OMEM / config_mem_perFrame) - 1; i >= 0;) {
        if (memory[i].pid != -1) {
            int pid = memory[i].pid;
            int start = i;

            // Find contiguous memory blocks for this process
            while (start >= 0 && memory[start].pid == pid) {
                start--;
            }

            // Print the process block with start and end addresses
            fprintf(snapshotFile, "%d\nP%d\n%d\n\n", (i + 1) * config_mem_perFrame, pid, (start + 1) * config_mem_perFrame);
            i = start; // Skip to the next unallocated block
        } else {
            i--; // Move to the next block if this one is free
        }
    }

    fprintf(snapshotFile, "====start==== = 0\n");
    fclose(snapshotFile);
}

//MCO2
void vmStat() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hConsole, 14); 
    printf("\n================================================\n");
    printf("|---------------> VMStat Report <--------------|\n"); 
    printf("================================================\n");

    SetConsoleTextAttribute(hConsole, 11); 
    printf("Total Memory: %d KB\n", config_MAX_OMEM);
    printf("Used Memory: %d KB\n", config_MAX_OMEM - free_memory);
    printf("Free Memory: %d KB\n", free_memory);

    // Display CPU ticks
    SetConsoleTextAttribute(hConsole, 10); 
    printf("Idle CPU Ticks: %d\n", idle_cpu_ticks);
    printf("Active CPU Ticks: %d\n", active_cpu_ticks);
    printf("Total CPU Ticks: %d\n", total_cpu_ticks);

    // Paging statistics
    SetConsoleTextAttribute(hConsole, 13); 
    printf("Num Paged In: %d\n", pages_paged_in);
    printf("Num Paged Out: %d\n", pages_paged_out);

    SetConsoleTextAttribute(hConsole, 14); 
    printf("\n================================================\n");

    SetConsoleTextAttribute(hConsole, 15); 
}

void displayMainMenuProcessSMI() {
    int cores_in_use = 0;
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WaitForSingleObject(process_lock, INFINITE); // Lock for thread safety

    // Calculate the number of cores in use
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break; // Move to the next core once a process is found
            }
        }
    }

    // Display ASCII Art and Header
    SetConsoleTextAttribute(hConsole, 14);
    printf("+-----------------------------------------------+\n");
    printf("|     PROCESS-SMI V01.00 Driver Version: 1.0    |\n");
    printf("+-----------------------------------------------+\n");
    
    // Memory Usage
    int used_memory = config_MAX_OMEM - free_memory;
    double memory_utilization = (float)(used_memory * 100) / config_MAX_OMEM;
    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100.0;
    SetConsoleTextAttribute(hConsole, 11);
    printf("CPU-Util: %.0f%%\n", cpu_utilization);
    printf("Memory Usage: %.2f MiB / %.2f MiB\n", used_memory / 1024.0f, config_MAX_OMEM / 1024.0f);
    printf("Memory Utilization: %.2f%%\n", memory_utilization);

    // Paging Statistics
    printf("\nPaging Statistics:\n");
    printf("Pages Paged In: %d\n", pages_paged_in);
    printf("Pages Paged Out: %d\n", pages_paged_out);

    // Fragmentation Details
    int fragmentation = calculateExternalFragmentation();
    printf("External Fragmentation: %d KB\n", fragmentation);

    // List running processes and their memory usage
    SetConsoleTextAttribute(hConsole, 13); 
    printf("\n================================================\n");
    printf("Running processes and memory usage:\n");
    printf("================================================\n");
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished) {
            SetConsoleTextAttribute(hConsole, 10); 
            printf("Process: %s (PID: %d)\n", processes[i].name, processes[i].pid);
            printf("Memory Allocated: %d KB\n", processes[i].memory_size);
            printf("Instructions Completed: %d / %d\n",
                   processes[i].instructions_completed, processes[i].instructions);
            printf("-----------------------------------------\n");
        }
    }

    ReleaseMutex(process_lock); // Unlock the mutex
    SetConsoleTextAttribute(hConsole, 15); // Reset console color
}

int calculateExternalFragmentation() {
    int external_fragmentation = 0;
    int contiguous_free_space = 0;
    bool in_free_block = false;

    for (int i = 0; i < config_MAX_OMEM / config_mem_perFrame; i++) {
        if (memory[i].pid == -1) {
            if (!in_free_block) {
                in_free_block = true;
            }
            contiguous_free_space += config_mem_perFrame;
        } else {
            if (in_free_block) {
                external_fragmentation += contiguous_free_space;
                contiguous_free_space = 0;
                in_free_block = false;
            }
        }
    }

    if (in_free_block) {
        external_fragmentation += contiguous_free_space;
    }

    return external_fragmentation;
}

void recordBackingStoreSnapshot(int cycle) {
    time_t rawtime;
    struct tm *timeinfo;
    char formattedTime[80];

    // Get current time for the timestamp
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(formattedTime, sizeof(formattedTime), "(%m/%d/%Y %I:%M:%S%p)", timeinfo);

    // Define folder for snapshots
    const char *folderPath = "./backing_store_snapshots/";
    _mkdir(folderPath); // Create the folder if it doesn't exist (Windows-specific)

    // Construct the full file path
    char filename[150];
    sprintf(filename, "%sbacking_store_%d.txt", folderPath, cycle);

    // Open the file for writing
    FILE *snapshotFile = fopen(filename, "w");
    if (!snapshotFile) {
        printf("Error: Unable to create backing store snapshot file '%s'.\n", filename);
        return;
    }

    // Write the timestamp and cycle number
    fprintf(snapshotFile, "Backing Store Snapshot - Cycle %d\n", cycle);
    fprintf(snapshotFile, "Timestamp: %s\n\n", formattedTime);

    // Write details about paging activity
    fprintf(snapshotFile, "Total Pages Paged In: %d\n", pages_paged_in);
    fprintf(snapshotFile, "Total Pages Paged Out: %d\n", pages_paged_out);

    // External fragmentation
    int fragmentation = calculateExternalFragmentation(); // Ensure this function is available
    fprintf(snapshotFile, "Total External Fragmentation: %d KB\n\n", fragmentation);

    // Write details of processes in the backing store
    fprintf(snapshotFile, "Processes in Backing Store:\n");
    for (int i = 0; i < process_count; i++) {
        if (processes[i].memory_size > 0) { // Assuming memory_size indicates backing store usage
            fprintf(snapshotFile, "Process: %s (PID: %d)\n", processes[i].name, processes[i].pid);
            fprintf(snapshotFile, "Memory Allocated: %d KB\n", processes[i].memory_size);
            fprintf(snapshotFile, "Instructions Completed: %d / %d\n\n",
                    processes[i].instructions_completed, processes[i].instructions);
        }
    }

    fclose(snapshotFile);
}

DWORD WINAPI backingStoreLogger(LPVOID lpParam) {
    int cycle = 0;  // Snapshot cycle counter
    while (scheduler_running) {
        Sleep(5000);  // Wait 5 seconds between snapshots (adjust as needed)
        recordBackingStoreSnapshot(cycle++);
    }
    return 0;
}

void startBackgroundTasks() {
    HANDLE loggerThread = CreateThread(NULL, 0, backingStoreLogger, NULL, 0, NULL);
    if (loggerThread == NULL) {
        printf("Error: Unable to create backing store logger thread.\n");
    }
}
