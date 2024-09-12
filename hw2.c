#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to clear the console screen
void clearScreen() {
    // Clears the console screen and reprints the header
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    printf("  _____    _____    ____    _____   ______   _____   __     __\n");
    printf(" / ____|  / ____|  / __ \\  |  __ \\ |  ____| / ____|  \\ \\   / /\n");
    printf("| |      | (___   | |  | | | |__) || |__   | (___     \\ \\_/ / \n");
    printf("| |       \\___ \\  | |  | | |  ___/ |  __|   \\___ \\     \\   /  \n");
    printf("| |____   ____) | | |__| | | |     | |____  ____) |     | |   \n");
    printf(" \\_____| |_____/   \\____/  |_|     |______||_____/      |_|   \n");

    printf("Hello, Welcome to CSOPESY commandline!\n");
    printf("Type 'exit' to quit, 'clear' to clear the screen\n");
}

// Function to handle the commands
void handleCommand(const char *command) {
    if (strcmp(command, "initialize") == 0) {
        printf("initialize command recognized. Doing something.\n");
    } else if (strcmp(command, "screen") == 0) {
        printf("screen command recognized. Doing something.\n");
    } else if (strcmp(command, "scheduler-test") == 0) {
        printf("scheduler-test command recognized. Doing something.\n");
    } else if (strcmp(command, "scheduler-stop") == 0) {
        printf("scheduler-stop command recognized. Doing something.\n");
    } else if (strcmp(command, "report-util") == 0) {
        printf("report-util command recognized. Doing something.\n");
    } else if (strcmp(command, "clear") == 0) {
        clearScreen();
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting...\n");
        exit(0);
    } else {
        printf("Command not recognized.\n");
    }
}
