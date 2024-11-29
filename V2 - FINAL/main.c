#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "controller.h"
#include "view.h"
#include "model.h"

int initialized = 0;  // Track if initialization has occurred
int in_marquee_mode = 0;
int in_main_menu = 1;

int main() {
    char command[100];

    while (1) {
        printf("Enter 'initialize' to start: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "initialize") == 0) {
            if (initializeSchedulerFromFile()) {  // If initialization is successful
                displayMainMenu();
                initialized = 1;  // Set initialized to true after successful initialization
                startBackgroundTasks();
                break;
            }
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting program...\n");
            exit(0);
        } else {
        	setConsoleColor(12);
            printf("Command unrecognized. Please try again.\n");
            resetConsoleColor();
        }
    }

    while (1) {
        printf("\nEnter a command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        handleCommand(command);
    }

    return 0;
}

