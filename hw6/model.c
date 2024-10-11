#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

Process processes[MAX_PROCESSES];
Screen screens[MAX_SCREENS];
int process_count = 0;
int current_process = 0;
int screen_count = 0;
HANDLE process_lock;

void initializeScheduler() {
    process_lock = CreateMutex(NULL, FALSE, NULL);
    process_count = 0;
    current_process = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process p;
        sprintf(p.name, "process_%d", i + 1);
        p.pid = i + 1;
        p.instructions = PRINT_COMMANDS;
        p.finished = 0;
        time_t t = time(NULL);
        struct tm* tm_info = localtime(&t);
        strftime(p.timestamp, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);
        processes[process_count++] = p;
    }
    
    printf("Scheduler initialized with %d processes.\n", process_count);
    
    for (int i = 0; i < process_count; i++) {
        printf("Initialized %s with PID %d\n", processes[i].name, processes[i].pid);
    }
}

void testScheduler() {
    HANDLE cores[MAX_CORES];
    int core_ids[MAX_CORES];

    printf("Starting the scheduler test...\n");
    
    for (int i = 0; i < MAX_CORES; i++) {
        core_ids[i] = i;
        cores[i] = CreateThread(NULL, 0, executeProcess, &core_ids[i], 0, NULL);
    }

    WaitForMultipleObjects(MAX_CORES, cores, TRUE, INFINITE);

    for (int i = 0; i < process_count; i++) {
        printf("Process %s (Core %d) finished execution.\n", processes[i].name, processes[i].core_id);
    }

    printf("Scheduler test completed.\n");
}

DWORD WINAPI executeProcess(LPVOID arg) {
    int core_id = *(int*)arg;
    
    while (1) {
        WaitForSingleObject(process_lock, INFINITE);
        
        if (current_process >= process_count) {
            ReleaseMutex(process_lock);
            break;
        }

        Process* process = &processes[current_process];
        process->core_id = core_id;
        current_process++;

        ReleaseMutex(process_lock);

        char filename[50];
        sprintf(filename, "process_%d_core_%d.txt", process->pid, core_id);
        FILE* log_file = fopen(filename, "w");
        if (log_file == NULL) {
            printf("Error creating file %s\n", filename);
            continue;
        }

        for (int i = 0; i < PRINT_COMMANDS; i++) {
            time_t t = time(NULL);
            struct tm* tm_info = localtime(&t);
            char buffer[50];
            strftime(buffer, 50, "%m/%d/%Y %I:%M:%S %p", tm_info);

            fprintf(log_file, "(%s) Core:%d \"Hello world from %s!\"\n", buffer, core_id, process->name);
            Sleep(100);
        }

        fclose(log_file);
        process->finished = 1;
    }
    return 0;
}

void createScreen(char* name) {
    if (screen_count >= MAX_SCREENS) {
        printf("Maximum screen count reached.\n");
        return;
    }
    strcpy(screens[screen_count].name, name);
    screen_count++;
    printf("Screen '%s' created.\n", name);
}

void resumeScreen(char* name) {
    for (int i = 0; i < screen_count; i++) {
        if (strcmp(screens[i].name, name) == 0) {
            printf("Resuming screen: %s\n", name);
            return;
        }
    }
    printf("Screen '%s' not found.\n", name);
}

void listScreens() {
    printf("Active Screens:\n");
    for (int i = 0; i < screen_count; i++) {
        printf("Screen %d: %s\n", i + 1, screens[i].name);
    }
}
