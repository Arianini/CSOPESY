#include "model.h"
#include "view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>

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

int initializeSchedulerFromFile() {
    FILE* config = fopen("config.txt", "r");
    if (!config) {
        setConsoleColor(12);
        printf("Error: 'config.txt' not found.\n");
        resetConsoleColor();
        return 0;
    }
    if (fscanf(config, "%d %s %d %f %d %d %f",
               &NUM_CORES,
               config_schedulerType,
               &config_quantumCycles,
               &config_batchProcessFreq,
               &config_minIns,
               &config_maxIns,
               &config_delayPerExec) != 7) {
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

DWORD WINAPI coreWorker(LPVOID arg) {
    int core_id = *(int*)arg;
    free(arg);

    while (scheduler_running) {
        if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) {
            return 0; // Exit if stop event is triggered
        }

        Process* current_process = NULL;

        WaitForSingleObject(process_lock, INFINITE); // Lock to safely find a process for this core
        for (int i = 0; i < process_count; i++) {
            if (!processes[i].finished && processes[i].core_id == -1) {
                current_process = &processes[i];
                current_process->core_id = core_id; // Assign the process to this core
                break;
            }
        }
        ReleaseMutex(process_lock);

        if (current_process == NULL) {
            Sleep(50); // No process to run; idle briefly and check again
            continue;
        }

        printf("Core %d is processing %s (PID: %d)\n", core_id, current_process->name, current_process->pid);

        // Execute the assigned process based on the scheduler type
        if (strcmp(config_schedulerType, "rr") == 0) {
            int quantum = config_quantumCycles;
            while (quantum > 0 && current_process->instructions_completed < current_process->instructions) {
                Sleep(config_delayPerExec * 100);
                current_process->instructions_completed++;
                quantum--;
                
                if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) return 0;
            }
        } else if (strcmp(config_schedulerType, "fcfs") == 0) {
            while (current_process->instructions_completed < current_process->instructions && scheduler_running) {
                Sleep(config_delayPerExec * 100);
                current_process->instructions_completed++;
            }
        }

        // Mark the process as finished if completed
        WaitForSingleObject(process_lock, INFINITE);
        if (current_process->instructions_completed >= current_process->instructions) {
            current_process->finished = 1;
        }
        current_process->core_id = -1; // Release the core for the next process
        ReleaseMutex(process_lock);

        printf("Core %d finished process %s (PID: %d)\n", core_id, current_process->name, current_process->pid);
    }

    return 0;
}

DWORD WINAPI generateProcesses(LPVOID lpParam) {
    int scheduler_process_count = 0;

    while (scheduler_running && !stop_generating) {
        // Check if we are allowed to generate a new batch of processes
        for (int i = 0; i < 1; i++) {
            // Break out of the loop if stop_generating is set
            if (stop_generating) {
                break;
            }

            if (process_count >= MAX_PROCESSES) {
                resizeProcessesArray();  // Resize array if at capacity
            }

            WaitForSingleObject(process_lock, INFINITE);  // Lock to safely create process

            Process* new_process = &processes[process_count++];
            snprintf(new_process->name, sizeof(new_process->name), "Process%02d", scheduler_process_count + 1);
            new_process->pid = process_count;
            new_process->core_id = -1;
            new_process->instructions = config_minIns + (rand() % (config_maxIns - config_minIns + 1));
            new_process->instructions_completed = 0;
            new_process->finished = 0;
            scheduler_process_count++;

            ReleaseMutex(process_lock);

            Sleep((int)(config_delayPerExec * 1000));  // Delay for next process within the batch
        }

        // Sleep between batches to control the generation frequency
        Sleep((int)(config_batchProcessFreq * 1000));
    }

    return 0;
}


void initializeRoundRobinScheduler() {
    // Sort processes by arrival time (AT)
    for (int i = 0; i < process_count - 1; i++) {
        for (int j = i + 1; j < process_count; j++) {
            if (processes[i].at > processes[j].at) {
                Process temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }

    printf("Processes sorted by arrival time for Round Robin scheduling.\n");
    rr_index = 0;  // Reset round-robin index
}

void roundRobinScheduler(int core_id) {
    int quantum = config_quantumCycles;
    bool all_finished = false;

    while (scheduler_running && !all_finished) {
        all_finished = true;
        Process* current_process = NULL;

        WaitForSingleObject(process_lock, INFINITE);

        // Find the next process that needs to run
        for (int i = 0; i < process_count; i++) {
            int idx = (rr_index + i) % process_count;

            // Check if the process is not finished and not currently assigned to a core
            if (!processes[idx].finished && processes[idx].core_id == -1) {
                current_process = &processes[idx];
                current_process->core_id = core_id;
                rr_index = (idx + 1) % process_count;  // Move round-robin index
                all_finished = false;
                break;
            }
        }

        ReleaseMutex(process_lock);

        if (all_finished) {
            break;  // All processes are complete
        }

        if (current_process) {
            int time_slice = quantum;

            while (time_slice > 0 && current_process->instructions_completed < current_process->instructions) {
                Sleep(config_delayPerExec * 100);  // Simulate execution time
                current_process->instructions_completed++;
                time_slice--;

                // Check if the stop event was triggered
                if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) {
                    return;  // Exit if stop event is triggered
                }
            }

            WaitForSingleObject(process_lock, INFINITE);

            // Check if the process is finished
            if (current_process->instructions_completed >= current_process->instructions) {
                current_process->finished = 1;
                current_process->last_core_id = core_id;
                current_process->core_id = -1;
            } else {
                // Process still has remaining instructions, so we remove it from the core
                // and place it back in the queue
                current_process->core_id = -1;
            }

            ReleaseMutex(process_lock);
        }
    }
}

void initializeFCFSScheduler() {
    // Sort processes by arrival time (AT)
    for (int i = 0; i < process_count - 1; i++) {
        for (int j = i + 1; j < process_count; j++) {
            if (processes[i].at > processes[j].at) {
                Process temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }

    printf("Processes sorted by arrival time for FCFS scheduling.\n");
}

void fcfsScheduler(int core_id) {
    while (scheduler_running) {
        Process* current_process = NULL;

        // Acquire lock to safely access shared resources
        WaitForSingleObject(process_lock, INFINITE);

        // Find the next available process that is not yet finished and not assigned to any core
        for (int i = global_current_process_index; i < process_count; i++) {
            if (!processes[i].finished && processes[i].core_id == -1) {
                current_process = &processes[i];
                current_process->core_id = core_id; // Assign to this core
                global_current_process_index = i + 1; // Move to next unprocessed index
                break;
            }
        }
        ReleaseMutex(process_lock);

        // If no process is available to run, wait and check again
        if (current_process == NULL) {
            Sleep(config_delayPerExec * 100);
            continue;
        }

        // Execute the process completely without printing intermediate progress
        while (current_process->instructions_completed < current_process->instructions && scheduler_running) {
            Sleep(config_delayPerExec * 100); // Simulate execution delay
            current_process->instructions_completed++;
        }

        // Mark as finished and release the core assignment
        WaitForSingleObject(process_lock, INFINITE);
        current_process->finished = 1;
        current_process->last_core_id = core_id; // Store the core where it finished
        current_process->core_id = -1; // Release core assignment
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
            printf("%s (PID: %d) Finished Core: %d  %d / %d\n",
                   processes[i].name, processes[i].pid, processes[i].last_core_id,
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

    FILE* reportFile = fopen("csopsey-log.txt", "w");
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
            fprintf(reportFile, "%s (PID: %d) Finished Core: %d  %d / %d\n",
                    processes[i].name, processes[i].pid, processes[i].last_core_id,
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

