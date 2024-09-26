#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SCREENS 10  // Max number of screens to handle

// Structure to store screen information
typedef struct {
    char name[50];
    int id;  // Unique ID for each screen
    int current_instruction;
    int total_instructions;
    char timestamp[50];  // Timestamp for when the screen was created
} Screen;

// Array to store multiple screens
Screen screens[MAX_SCREENS];
int screen_count = 0;  // Track the number of created screens
int in_main_menu = 1;  // Flag to check if we are in the main menu

// Function to clear the console screen
void clearScreen() {
    // Clears the console screen
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Function to display the process screen
void displayScreen(Screen* screen) {
    clearScreen();
    printf("Process: %s\n", screen->name);
    printf("ID: %d\n", screen->id);  // Use the unique ID stored for each screen
    printf("Current instruction line: %d/%d\n", screen->current_instruction, screen->total_instructions);
    printf("Screen created on: %s\n", screen->timestamp);  // Display the creation timestamp
}

// Function to create a new screen with the given name
void createScreen(char* name) {
    if (screen_count >= MAX_SCREENS) {
        printf("Maximum screen limit reached.\n");
        return;
    }

    // Create a new screen 
    strcpy(screens[screen_count].name, name);
    screens[screen_count].id = screen_count + 1;  // Assign a unique ID starting from 1
    screens[screen_count].current_instruction = 0;  // Start from instruction 0
    screens[screen_count].total_instructions = 50;  // Simulating a total of 50 lines of code

    // Get current timestamp
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(screens[screen_count].timestamp, 50, "%m/%d/%Y, %I:%M:%S %p", tm_info);

    // Switch to the new screen
    screen_count++;  // Increment screen count
    displayScreen(&screens[screen_count - 1]);
    in_main_menu = 0;  // We are no longer in the main menu
}

// Function to resume an existing screen by name
void resumeScreen(char* name) {
    int found = 0;  // Flag to check if screen was found
    for (int i = 0; i < screen_count; i++) {
        if (strcmp(screens[i].name, name) == 0) {
            displayScreen(&screens[i]);  // Display the found screen with the correct ID
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Screen '%s' not found.\n", name);
        if (in_main_menu) {
            printf("Enter a command: ");
        } else {
            printf("root:\\> ");
        }
    } else {
        in_main_menu = 0;  // We are no longer in the main menu after resuming a screen
    }
}

// Function to display the main menu
void displayMainMenu() {
    clearScreen();
    printf("  _____    _____    ____    _____   ______   _____   __     __\n");
    printf(" / ____|  / ____|  / __ \\  |  __ \\ |  ____| / ____|  \\ \\   / /\n");
    printf("| |      | (___   | |  | | | |__) || |__   | (___     \\ \\_/ / \n");
    printf("| |       \\___ \\  | |  | | |  ___/ |  __|   \\___ \\     \\   /  \n");
    printf("| |____   ____) | | |__| | | |     | |____  ____) |     | |   \n");
    printf(" \\_____| |_____/   \\____/  |_|     |______||_____/      |_|   \n");

    printf("Hello, Welcome to CSOPESY commandline!\n");
    printf("Type 'exit' to return to the main menu, 'clear' to clear the screen\n");
    printf("Enter a command: ");
    in_main_menu = 1;  // Set flag that we are in the main menu
}

// Function to handle the commands in the main menu
void handleMainMenuCommand(const char* command) {
    if (strcmp(command, "initialize") == 0) {
        printf("Initialize command recognized. Doing something.\n");
    } else if (strcmp(command, "screen") == 0) {
        printf("Screen command recognized. Doing something.\n");
    } else if (strcmp(command, "scheduler-test") == 0) {
        printf("Scheduler-test command recognized. Doing something.\n");
    } else if (strcmp(command, "scheduler-stop") == 0) {
        printf("Scheduler-stop command recognized. Doing something.\n");
    } else if (strcmp(command, "report-util") == 0) {
        printf("Report-util command recognized. Doing something.\n");
    } else if (strcmp(command, "clear") == 0) {
        clearScreen();
        displayMainMenu();
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting program...\n");
        exit(0);
    } else {
        printf("Command not recognized.\n");
    }
}

// Function to handle commands including screens
void handleCommand(char* command) {
    if (in_main_menu) {
        if (strncmp(command, "screen -s", 9) == 0) {
            // Extract screen name and create a screen
            char screen_name[50];
            sscanf(command, "screen -s %s", screen_name);
            createScreen(screen_name);
        } else if (strncmp(command, "screen -r", 9) == 0) {
            // Extract screen name and resume it
            char screen_name[50];
            sscanf(command, "screen -r %s", screen_name);
            resumeScreen(screen_name);
        } else {
            // Handle other main menu commands
            handleMainMenuCommand(command);
        }
    } else {
        // Only 'exit' is allowed in the process screen
        if (strcmp(command, "exit") == 0) {
            displayMainMenu();  // Return to the main menu
        } else {
            printf("Only the 'exit' command works in this mode.\n");
        }
    }
}

int main() {
    char command[100];

    // Print the header
    displayMainMenu();

    // Main loop to accept user input
    while (1) {
        printf(in_main_menu ? "Enter a command: " : "root:\\> ");
        fgets(command, sizeof(command), stdin);
        // Remove newline character from fgets
        command[strcspn(command, "\n")] = 0;

        // Handle the command
        handleCommand(command);
    }

    return 0;
}
