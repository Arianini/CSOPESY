#include "controller.h"
#include "model.h"       
#include "view.h"
#include <stdio.h>
#include <string.h>

// Handles user commands and dispatches them to appropriate actions.
void handleCommand(char* command) {
    if (strncmp(command, "initialize", 10) == 0) {
        initializeScheduler();
    } else if (strncmp(command, "scheduler-stop", 14) == 0) {
        stopScheduler();
    } else if (strncmp(command, "scheduler-test", 14) == 0) {
        // Placeholder for scheduler-test functionality
        printf("Running scheduler test...\n");
        // Implement test functionality here
    } else if (strncmp(command, "screen -ls", 10) == 0) {
        if (scheduler_running) {
            printProcessLogs(processes, process_count);
        } else {
            printf("Scheduler is stopped. No processes to display.\n");
        }
    } else if (strncmp(command, "screen -s", 9) == 0) {
        char screen_name[50];
        sscanf(command, "screen -s %s", screen_name);
        createScreen(screen_name);
    } else if (strncmp(command, "screen -r", 9) == 0) {
        char screen_name[50];
        sscanf(command, "screen -r %s", screen_name);
        resumeScreen(screen_name);
    } else if (strcmp(command, "report-util") == 0) {
        // Placeholder for report-util functionality
        printf("Generating utilization report...\n");
        // Implement report functionality here
    } else if (strcmp(command, "help") == 0) {
        // Display list of available commands
        printf("\nAvailable commands:\n");
        printf("  initialize       - Initialize the scheduler\n");
        printf("  scheduler-stop   - Stop the scheduler\n");
        printf("  scheduler-test   - Run a scheduler test\n");
        printf("  screen -ls       - List running and finished processes\n");
        printf("  screen -s <name> - Create a screen with the given name\n");
        printf("  screen -r <name> - Resume an existing screen by name\n");
        printf("  report-util      - Generate utilization report\n");
        printf("  clear            - Clear the screen\n");
        printf("  help             - Show this help message\n");
        printf("  exit             - Exit the program\n");
    } else if (strcmp(command, "clear") == 0) {
        // Call clearScreen function to clear the terminal screen
        clearScreen();
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting program...\n");
        exit(0);
    } else {
        printf("Command '%s' not recognized.\n", command);
    }
}

