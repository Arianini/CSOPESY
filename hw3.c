#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SCREENS 10  // Define a maximum number of screens to handle

// Structure to store screen information
typedef struct {
    char name[50];
    int current_instruction;
    int total_instructions;
    char timestamp[50];
} Screen;

// Array to store multiple screens
Screen screens[MAX_SCREENS];

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

// Function to create a new screen with the given name
void createScreen(char* name) 
{
	int i; 
	int screen_count = 0; // Track the number of created screens
	
    // Create a new screen 
    strcpy(screens[screen_count].name, name);
    screens[screen_count].current_instruction = 0;  // Start from instruction 0
    screens[screen_count].total_instructions = 100;  // Simulating a total of 100 instructions

    // Get current timestamp
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(screens[screen_count].timestamp, 50, "%m/%d/%Y, %I:%M:%S %p", tm_info);

    // Display screen information
    printf("Screen '%s' created.\n", name);
    printf("Instructions: %d/%d\n", screens[screen_count].current_instruction, screens[screen_count].total_instructions);
    printf("Timestamp: %s\n", screens[screen_count].timestamp);

    screen_count++;  // Increment screen count 
}


// Function to resume an existing screen by name
void resumeScreen(char* name) 
{
    
}

// Function to handle the commands
void handleCommand(char *command) 
{
    if (strcmp(command, "initialize") == 0) 
	{
        printf("initialize command recognized.\n");
    } else if (strncmp(command, "screen -s", 9) == 0) 
	{
        // Extract screen name and create a screen
        char screen_name[50];
        sscanf(command, "screen -s %s", screen_name);
        createScreen(screen_name);
    } else if (strcmp(command, "scheduler-test") == 0) {
        printf("scheduler-test command recognized.\n");
    } else if (strcmp(command, "scheduler-stop") == 0) {
        printf("scheduler-stop command recognized.\n");
    } else if (strcmp(command, "report-util") == 0) {
        printf("report-util command recognized.\n");
    } else if (strcmp(command, "clear") == 0) {
        clearScreen();
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting...\n");
        exit(0);
    } else {
        printf("Command not recognized.\n");
    }
}

int main() {
    char command[100];

    // Print the header
    clearScreen();

    // Main loop to accept user input
    while (1) {
        printf("Enter a command: ");
        fgets(command, sizeof(command), stdin);
        // Remove newline character from fgets
        command[strcspn(command, "\n")] = 0;

        // Handle the command
        handleCommand(command);
    }

    return 0;
}
