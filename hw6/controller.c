#include "controller.h"
#include "model.h"
#include "view.h"
#include <stdio.h>
#include <string.h>

void handleCommand(char* command) {
    if (strncmp(command, "initialize", 10) == 0) {
        initializeScheduler();
    } else if (strncmp(command, "scheduler-test", 14) == 0) {
        testScheduler();
    } else if (strncmp(command, "screen -s", 9) == 0) {
        char screen_name[50];
        sscanf(command, "screen -s %s", screen_name);
        createScreen(screen_name);
    } else if (strncmp(command, "screen -r", 9) == 0) {
        char screen_name[50];
        sscanf(command, "screen -r %s", screen_name);
        resumeScreen(screen_name);
    } else if (strncmp(command, "report-util", 11) == 0) {
        printProcessLogs(processes, process_count);
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting program...\n");
        exit(0);
    } else {
        printf("Command '%s' not recognized.\n", command);
    }
}

