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
Screen *screens = NULL;
int process_count = 0;
int current_process = 0;
int screen_count = 0;
HANDLE process_lock;
HANDLE stop_event;
int scheduler_running = 0;
int global_current_process_index = 0;
static int rr_index = 0;
int in_scheduler_test = 0; // Flag to suppress print statements during scheduler test


int initializeSchedulerFromFile() {
    FILE* config = fopen("config.txt", "r");
    // Check if file exists
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

    processes = (Process*)malloc(sizeof(Process) * MAX_PROCESSES);
    screens = (Screen*)malloc(sizeof(Screen) * MAX_SCHEDULER_PROCESSES); 
    if (!processes || !screens) {
        printf("Error: Memory allocation failed.\n");
        return 0;
    }

    process_lock = CreateMutex(NULL, FALSE, NULL);
    stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    scheduler_running = 1;
    return 1;
}

void runSchedulerTest() {
    printf("Running scheduler test...\n");

    in_scheduler_test = 1;  // Suppress output during test

    // Reset state
    current_process = 0;
    scheduler_running = 1;
    ResetEvent(stop_event);

    // Initialize processes (example processes)
    int scheduler_process_count = 0;
    while (scheduler_process_count < MAX_SCHEDULER_PROCESSES) {
        // Dynamically increase memory allocation if needed
        if (process_count >= MAX_PROCESSES) {
            Process* new_memory = realloc(processes, sizeof(Process) * (process_count + 1));
            if (!new_memory) {
                printf("Error: Memory allocation failed for additional process.\n");
                break;
            }
            processes = new_memory;
        }
        
        Process* p = &processes[process_count];
        sprintf(p->name, "process%02d", scheduler_process_count + 1);
        p->pid = process_count + 1;
        p->core_id = -1;
        p->instructions = config_minIns + (rand() % (config_maxIns - config_minIns + 1));
        p->bt = p->instructions;
        p->instructions_completed = 0;
        p->finished = 0;
        p->at = scheduler_process_count * 10;
        
        process_count++;
        scheduler_process_count++;
        
        Sleep(config_delayPerExec * 1000);
    }

    if (strcmp(config_schedulerType, "rr") == 0) {
        initializeRoundRobinScheduler();
    } else {
        initializeFCFSScheduler();
    }

    // Start threads for each core
    for (int i = 0; i < NUM_CORES; ++i) {
        int* core_id = malloc(sizeof(int));
        *core_id = i;
        HANDLE thread = CreateThread(NULL, 0, executeProcess, core_id, 0, NULL);
        if (thread) CloseHandle(thread);
    }

    printf("Scheduler test started with %d processes.\n", process_count);
    in_scheduler_test = 0;  // Reset flag after test
}

void stopScheduler() {
    if (scheduler_running) {
        printf("Stopping scheduler...\n");
        SetEvent(stop_event); // Signal threads to stop
        scheduler_running = 0;
        Sleep(100); // Give threads time to stop
    } else {
        printf("Scheduler is already stopped.\n");
    }
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

        Sleep(config_delayPerExec * 1000);  // Delay to prevent CPU overuse
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
                Sleep(config_delayPerExec * 1000);  // Simulate execution time
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
            Sleep(config_delayPerExec * 1000);
            continue;
        }

        // Execute the process completely without printing intermediate progress
        while (current_process->instructions_completed < current_process->instructions && scheduler_running) {
            Sleep(config_delayPerExec * 1000); // Simulate execution delay
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
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break;
            }
        }
    }

    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100;
    printf("CPU utilization: %.0f%%\n", cpu_utilization);
    printf("Cores used: %d\n", cores_in_use);
    printf("Cores available: %d\n", NUM_CORES - cores_in_use);
    printf("+-----------------------------------------------+\n");

    // Display running processes
    printf("\033[1;36mRunning processes:\033[0m\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            printf("%s (PID: %d) Core: %d  %d / %d\n", 
                   processes[i].name, processes[i].pid, processes[i].core_id, 
                   processes[i].instructions_completed, processes[i].instructions);
            running_found = 1;
        }
    }
    if (!running_found) {
        printf("No running processes.\n");
    }

    // Display finished processes
    printf("\033[1;36mFinished processes:\033[0m\n");
    int finished_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            printf("%s (PID: %d) Finished Core: %d  %d / %d\n",
                   processes[i].name, processes[i].pid, processes[i].last_core_id,
                   processes[i].instructions_completed, processes[i].instructions);
            finished_found = 1;
        }
    }
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
        Sleep(config_delayPerExec * 1000);  // Multiply by 1000 to convert to milliseconds
    }
    
    return 0;
}

// Check for an available core; return -1 if none is available
int getAvailableCore() {
    for (int i = 0; i < NUM_CORES; i++) {
        int core_in_use = 0;
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                core_in_use = 1;
                break;
            }
        }
        if (!core_in_use) return i;  // Return the ID of the available core
    }
    return -1;  // No core available
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
                    Sleep(config_delayPerExec * 1000);
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
                    Sleep(config_delayPerExec * 1000);
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
    Process* new_memory = realloc(processes, sizeof(Process) * (process_count + 1));
    if (!new_memory) {
        printf("Error: Memory allocation failed for additional process.\n");
        return NULL;
    }
    processes = new_memory;

    // Define `new_process` as the address of the newly added process slot
    Process* new_process = &processes[process_count];
    
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';  // Ensure null termination
    new_process->pid = process_count + 1;  
    new_process->core_id = -1;  // Not assigned initially
    new_process->instructions = config_minIns + (rand() % (config_maxIns - config_minIns + 1));
    new_process->bt = new_process->instructions;
    new_process->instructions_completed = 0;
    new_process->finished = 0;
    new_process->at = 0;
    time_t t = time(NULL);
    strftime(new_process->timestamp, sizeof(new_process->timestamp), "%m/%d/%Y %I:%M:%S %p", localtime(&t));

    printf("New process '%s' created with PID %d.\n", new_process->name, new_process->pid);

    // Start the process in a separate thread based on scheduler configuration
    HANDLE thread = CreateThread(NULL, 0, startManualProcess, new_process, 0, NULL);
    if (thread) CloseHandle(thread);

    process_count++;  // Increment the total process count

    return new_process;
}

void createScreen(char* process_name) {
    if (screen_count >= MAX_SCREENS) {
        printf("Maximum screen limit reached.\n");
        return;
    }

    // Find the process by name
    Process* process = getProcessByName(process_name);
    if (!process) {
        printf("Error: Process '%s' not found.\n", process_name);
        return;
    }

    // Initialize a new screen for the found process
    Screen new_screen;
    strcpy(new_screen.name, process_name);
    new_screen.id = screen_count + 1;
    new_screen.current_instruction = process->instructions_completed;
    new_screen.total_instructions = process->instructions;
    new_screen.created_by_screen_s = true;
    
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(new_screen.timestamp, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);

    screens[screen_count++] = new_screen;

    // Switch to this screen and display process details
    in_main_menu = 0;
    displayScreen(&new_screen);
}

#include <conio.h>  // For _kbhit() and _getch()

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

            // Note: Do not loop or stay in this screen mode; return to main menu after displaying
            in_main_menu = 1;
            displayMainMenu();
            break;
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
        printf("Error: Could not create or open 'utilization_report.txt'.\n");
        return;
    }

    fprintf(reportFile, "CPU Utilization Report\n");
    fprintf(reportFile, "======================\n");

    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    char timeBuffer[50];
    strftime(timeBuffer, sizeof(timeBuffer), "%m/%d/%Y %I:%M:%S %p", tm_info);
    fprintf(reportFile, "Report generated on: %s\n\n", timeBuffer);

    // Lock the process data to get accurate CPU utilization
    WaitForSingleObject(process_lock, INFINITE);

    int cores_in_use = 0;
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break;
            }
        }
    }

    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100;
    fprintf(reportFile, "CPU Utilization: %.2f%%\n", cpu_utilization);
    fprintf(reportFile, "Cores in use: %d / %d\n\n", cores_in_use, NUM_CORES);

    fprintf(reportFile, "Running Processes:\n");
    fprintf(reportFile, "-------------------\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            fprintf(reportFile, "Process Name: %s\n", processes[i].name);
            fprintf(reportFile, "PID: %d, Core ID: %d\n", processes[i].pid, processes[i].core_id);
            fprintf(reportFile, "Completed Instructions: %d / %d\n", processes[i].instructions_completed, processes[i].instructions);
            fprintf(reportFile, "Started at: %s\n\n", processes[i].timestamp);
            running_found = 1;
        }
    }
    if (!running_found) {
        fprintf(reportFile, "No running processes.\n");
    }

    fprintf(reportFile, "Finished Processes:\n");
    fprintf(reportFile, "--------------------\n");
    int finished_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            fprintf(reportFile, "Process Name: %s\n", processes[i].name);
            fprintf(reportFile, "PID: %d, Core ID: %d\n", processes[i].pid, processes[i].core_id);
            fprintf(reportFile, "Completed Instructions: %d / %d\n", processes[i].instructions_completed, processes[i].instructions);
            fprintf(reportFile, "Finished at: %s\n\n", processes[i].timestamp);
            finished_found = 1;
        }
    }
    if (!finished_found) {
        fprintf(reportFile, "No finished processes.\n");
    }

    // Release the lock after data is read
    ReleaseMutex(process_lock);

    fclose(reportFile);
    printf("Utilization report generated in 'csopsey-log.txt'.\n");
}
