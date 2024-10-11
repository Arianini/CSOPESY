#include "view.h"
#include <stdio.h>
#include <stdlib.h>

// Clears the console screen 
void clearScreen() {
    system("cls"); 
}

// Displays the main menu in the CLI.
void displayMainMenu() {
    clearScreen();
    printf("  _____    _____    ____    _____   ______   _____   __     __\n");
    printf(" / ____|  / ____|  / __ \\  |  __ \\ |  ____| / ____|  \\ \\   / /\n");
    printf("| |      | (___   | |  | | | |__) || |__   | (___     \\ \\_/ / \n");
    printf("| |       \\___ \\  | |  | | |  ___/ |  __|   \\___ \\     \\   /  \n");
    printf("| |____   ____) | | |__| | | |     | |____  ____) |     | |   \n");
    printf(" \\_____| |_____/   \\____/  |_|     |______||_____/      |_|   \n");
    printf("Hello, Welcome to CSOPESY commandline!\n");
   	printf("Type 'help' to view available commands, 'exit' to quit, 'clear' to clear the screen\n");
}

// Prints logs of running, finished, and unexecuted processes
void printProcessLogs(Process processes[], int process_count) {
    printf("+-----------------------------------------------+\n");
    
    // Display running processes
    printf("Running processes:\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            printf("%s (%s) Core: %d  %d / %d\n", 
                   processes[i].name, processes[i].timestamp, 
                   processes[i].core_id, 
                   PRINT_COMMANDS - processes[i].instructions, 
                   PRINT_COMMANDS);
            running_found = 1;
        }
    }
    if (!running_found) {
        printf("No running processes.\n");
    }

    // Display finished processes
    printf("\nFinished processes:\n");
    int finished_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished && processes[i].core_id >= 0) {
            printf("%s (%s) Finished on Core: %d  %d / %d\n", 
                   processes[i].name, processes[i].timestamp,
                   processes[i].core_id,
                   PRINT_COMMANDS, PRINT_COMMANDS);
            finished_found = 1;
        }
    }
    if (!finished_found) {
        printf("No finished processes.\n");
    }

    printf("+-----------------------------------------------+\n");
}

