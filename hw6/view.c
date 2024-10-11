#include "view.h"
#include <stdio.h>
#include <stdlib.h>

void clearScreen() {
    system("cls");  // Clears screen on Windows
}

void displayMainMenu() {
    clearScreen();
    printf("  _____    _____    ____    _____   ______   _____   __     __\n");
    printf(" / ____|  / ____|  / __ \\  |  __ \\ |  ____| / ____|  \\ \\   / /\n");
    printf("| |      | (___   | |  | | | |__) || |__   | (___     \\ \\_/ / \n");
    printf("| |       \\___ \\  | |  | | |  ___/ |  __|   \\___ \\     \\   /  \n");
    printf("| |____   ____) | | |__| | | |     | |____  ____) |     | |   \n");
    printf(" \\_____| |_____/   \\____/  |_|     |______||_____/      |_|   \n");
    printf("Hello, Welcome to CSOPESY commandline!\n");
    printf("Type 'exit' to return to the main menu, 'clear' to clear the screen, or 'nvidia-smi' to display GPU info\n");
}

void printProcessLogs(Process processes[], int process_count) {
    printf("Running processes:\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished) {
            printf("%s (Core %d) - Running\n", processes[i].name, processes[i].core_id);
            running_found = 1;
        }
    }
    if (!running_found) {
        printf("No running processes.\n");
    }

    printf("\nFinished processes:\n");
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            printf("%s (Core %d) - Finished\n", processes[i].name, processes[i].core_id);
        }
    }
}

