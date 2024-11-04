#include "controller.h"
#include "model.h"
#include "view.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int in_main_menu;
extern int in_marquee_mode;
extern Process *current_screen_process;  // Pointer to the current process being viewed in screen mode

void handleCommand(char* command) {
    char screen_name[50];

    if (in_main_menu) {
        if (strncmp(command, "screen -s", 9) == 0) {
            char screen_name[50];
            sscanf(command, "screen -s %49s", screen_name);
             // Check if the process with the given name exists
            current_screen_process = getProcessByName(screen_name); // Function in model.c
            if (current_screen_process) {
                in_main_menu = 0;  // Switch to screen mode
                createScreen(screen_name);
            } else {
                printf("Process '%s' not found in scheduler. Creating new process.\n", screen_name);
                current_screen_process = createNewProcess(screen_name);
                if (current_screen_process) {
                    in_main_menu = 0;
                    createScreen(screen_name);
                } else {
                    printf("Error: Could not create new process '%s'.\n", screen_name);
                }
            }
        } else if (strncmp(command, "screen -r", 9) == 0) {
            sscanf(command, "screen -r %49s", screen_name);
            current_screen_process = getProcessByName(screen_name);  // Find the process

            if (current_screen_process) {
                // Check if the process is already finished
                if (current_screen_process->finished) {
                    printf("Error: Process '%s' not found.\n", screen_name);
                    in_main_menu = 1;
                } else if (getScreenByName(screen_name) && getScreenByName(screen_name)->created_by_screen_s) {
                    in_main_menu = 0;  // Switch to screen mode
                    resumeScreen(screen_name);
                    displayScreen(getScreenByName(screen_name));
                } else {
                    printf("Error: Process '%s' not found.\n", screen_name);
                    in_main_menu = 1;
                }
            } else {
                printf("Error: Process '%s' not found.\n", screen_name);
                    in_main_menu = 1;
            }
        } else if (strcmp(command, "scheduler-test") == 0) {
            runSchedulerTest();
        } else if (strcmp(command, "scheduler-stop") == 0) {
            stopScheduler();
        } else if (strcmp(command, "screen -ls") == 0) {
            reportProgress();
        } else if (strcmp(command, "report-util") == 0) {
            generateUtilizationReport();
        } else if (strcmp(command, "help") == 0) {
            displayHelp();
        } else if (strcmp(command, "marquee") == 0) {
            displayMarquee();
        } else if (strcmp(command, "clear") == 0) {
            clearScreen();
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting program...\n");
            exit(0);
        } else {
            printf("Command '%s' not recognized.\n", command);
        }
    } else if (current_screen_process) {  // If in screen mode for a specific process
        if (strcmp(command, "process-smi") == 0) {
            displayProcessSMI(current_screen_process->name);  // Display only current process info
        } else if (strcmp(command, "exit") == 0) {
            in_main_menu = 1;   // Return to main menu
            current_screen_process = NULL;  // Clear current screen process
            displayMainMenu();
        } else {
            printf("Only 'process-smi' or 'exit' are allowed in this mode.\n");
        }
    }
}