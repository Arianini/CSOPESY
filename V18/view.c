#include "view.h"
#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <conio.h>
#include <process.h>

extern int in_marquee_mode;
extern int in_main_menu;
extern Screen screens[];
extern int screen_count;

// Global variables for marquee
int x = 0, y = 0, xDirection = 1, yDirection = 1;
int consoleWidth, consoleHeight;
const char message[] = "Hello world in marquee!";
char command[100] = "";

// Function to set console text color
void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Function to reset console text color to default (usually white)
void resetConsoleColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 15); // 15 is the default console color in Windows (white)
}

void displayScreen(Screen* screen) {
    Process* process = getProcessByName(screen->name);
    if (process) {
        system("cls");
        printf("Process: %s\n", screen->name);
        printf("ID: %d\n", screen->id);
        printf("Current instruction line: %d/%d\n", process->instructions_completed, process->instructions);
        printf("Screen created on: %s\n", screen->timestamp);

        if (process->finished) {
            printf("Status: Finished!\n");
        }
    }
}


void displayProcessSMI(const char* process_name) {
    WaitForSingleObject(process_lock, INFINITE); // Lock for thread safety

    Process* process = NULL;

    // Search for the specified process by name in the `processes` array
    for (int i = 0; i < process_count; i++) {
        if (strcmp(processes[i].name, process_name) == 0) {
            process = &processes[i];
            break;
        }
    }

    if (process->instructions_completed >= process->instructions) {
            printf("Finished!\n");
    } else if (process) {
        // Display the real-time status of the found process
        printf("Process: %s\n", process->name);
        printf("ID: %d\n", process->pid);
        printf("Current instruction line: %d\n", process->instructions_completed);
        printf("Total lines of code: %d\n", process->instructions);

        // Indicate if the process is finished
    } else {
        printf("Error: No active process found with the name '%s'.\n", process_name);
    }

    ReleaseMutex(process_lock); // Release lock
}



void clearScreen() {
    system("cls");
    displayMainMenu();
}

void displayMainMenu() {
    system("cls");
    printf("\033[1;34m"); // Set text to blue
    printf("  _____    _____    ____    _____   ______   _____   __     __\n");
    printf(" / ____|  / ____|  / __ \\  |  __ \\ |  ____| / ____|  \\ \\   / /\n");
    printf("| |      | (___   | |  | | | |__) || |__   | (___     \\ \\_/ / \n");
    printf("| |       \\___ \\  | |  | | |  ___/ |  __|   \\___ \\     \\   /  \n");
    printf("| |____   ____) | | |__| | | |     | |____  ____) |     | |   \n");
    printf(" \\_____| |_____/   \\____/  |_|     |______||_____/      |_|   \n");
    printf("\033[0m"); // Reset text color
    printf("Hello, Welcome to CSOPESY commandline!\n");
    printf("Type 'help' to view available commands, 'exit' to quit, 'clear' to clear the screen\n");

    // Print the loaded configuration details
    printf("\033[1;32m"); // Set text to green
    printf("\nConfiguration loaded:\n");
    printf("  Number of CPUs: %d\n", NUM_CORES);
    printf("  Scheduler Type: %s\n", config_schedulerType);
    printf("  Quantum Cycles: %d\n", config_quantumCycles);
    printf("  Batch Process Frequency: %.2f\n", config_batchProcessFreq);
    printf("  Min Instructions: %d\n", config_minIns);
    printf("  Max Instructions: %d\n", config_maxIns);
    printf("  Delay Per Execution: %.2f\n", config_delayPerExec);
    printf("\033[0m\n"); // Reset text color
}

void displayProcessLogs() {
    printf("\033[1;33m+-----------------------------------------------+\n");

    // Calculate the number of used and available cores
    int cores_in_use = 0;
    for (int i = 0; i < NUM_CORES; i++) {
        for (int j = 0; j < process_count; j++) {
            if (processes[j].core_id == i && !processes[j].finished) {
                cores_in_use++;
                break;
            }
        }
    }

    double cpu_utilization = ((double)cores_in_use / NUM_CORES) * 100;
    printf("CPU utilization: %.0f%%\n", cpu_utilization);
    printf("Cores used: %d\n", cores_in_use);
    printf("Cores available: %d\n", NUM_CORES - cores_in_use);
    printf("+-----------------------------------------------+\033[0m\n");

    // Display running processes
    printf("\033[1;36mRunning processes:\033[0m\n");
    int running_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (!processes[i].finished && processes[i].core_id >= 0) {
            printf("%s (%s) Core: %d  %d / %d\n", 
                   processes[i].name, processes[i].timestamp, 
                   processes[i].core_id, 
                   processes[i].instructions_completed,
                   processes[i].instructions);
            running_found = 1;
        }
    }
    if (!running_found) {
        printf("No running processes.\n");
    }

    // Display finished processes
    printf("\033[1;36mFinished processes:\033[0m\n");
    int finished_found = 0;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].finished) {
            printf("%s (%s) Finished on Core: %d  %d / %d\n",
                   processes[i].name,
                   processes[i].timestamp,
                   processes[i].core_id,
                   processes[i].instructions_completed, 
                   processes[i].instructions);
            finished_found = 1;
        }
    }
    if (!finished_found) {
        printf("No finished processes.\n");
    }

    printf("\033[1;33m+-----------------------------------------------+\033[0m\n");
}

void displayHelp() {
    printf("\033[1;35m\nAvailable commands:\033[0m\n");
    printf("  scheduler-stop   - Stop the scheduler\n");
    printf("  scheduler-test   - Run a scheduler test\n");
    printf("  screen -ls       - List running and finished processes\n");
    printf("  screen -s <name> - Create a screen with the given name\n");
    printf("  screen -r <name> - Resume an existing screen by name\n");
    printf("  report-util      - Generate utilization report\n");
    printf("  marquee          - Display a marquee message\n");
    printf("  clear            - Clear the screen\n");
    printf("  help             - Show this help message\n");
    printf("  exit             - Exit the program\n");
}

void clearLine(int y) {
    COORD coord;
    coord.X = 0;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    for (int i = 0; i < 80; i++) printf(" ");
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void simulateHeavyWorkload() {
    for (int i = 0; i < 50000; i++) {
        (void)(i * i);
    }
}

void displayCommandInput(const char *command) {
    COORD coord;
    coord.X = 0;
    coord.Y = consoleHeight - 2;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    printf("Enter a command for MARQUEE_CONSOLE: %s", command);
}

void getConsoleSize(int *width, int *height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void handleInput(void* param) {
    while (1) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') {
                printf("\nCommand processed in MARQUEE_CONSOLE: %s\n", command);
                command[0] = '\0';
                displayCommandInput(command);
            } else if (ch == '\b') {
                int len = strlen(command);
                if (len > 0) {
                    command[len - 1] = '\0';
                    displayCommandInput(command);
                }
            } else {
                int len = strlen(command);
                if (len < sizeof(command) - 1) {
                    command[len] = ch;
                    command[len + 1] = '\0';
                    displayCommandInput(command);
                }
            }
        }

        Sleep(50);
    }
}

void displayMarqueeWithDesign(const char *message, int x, int y, int consoleWidth, int consoleHeight) {
    clearScreen();

    // Display the top and bottom borders of the marquee area
    printf("**************************************\n");
    printf("* Displaying marquee console! *\n");
    printf("**************************************\n");

    for (int i = 0; i < y; i++) {
        printf("\n");
    }

    for (int i = 0; i < x; i++) {
        printf(" ");
    }

    printf("%s\n", message);

    for (int i = 0; i < consoleHeight - y - 6; i++) {
        printf("\n");
    }

    displayCommandInput(command);
}

void marqueeMovement(void* param) {
    int length = strlen(message);

    while (in_marquee_mode) {
        getConsoleSize(&consoleWidth, &consoleHeight);
        displayMarqueeWithDesign(message, x, y, consoleWidth, consoleHeight);

        // Change direction if hitting the edge
        if (x + length >= consoleWidth) {
            xDirection = -1; // Move left
        } else if (x <= 0) {
            xDirection = 1; // Move right
        }

        if (y >= consoleHeight - 7) {
            yDirection = -1; // Move up
        } else if (y <= 0) {
            yDirection = 1; // Move down
        }

        // Update position
        x += xDirection;
        y += yDirection;

        // Adjust the delay to control the speed of the marquee
        Sleep(100);
    }
}

void displayMarquee() {
    in_marquee_mode = 1;
    in_main_menu = 0; // Explicitly set to avoid menu interference

    // Start the marquee movement in a separate thread
    _beginthread(marqueeMovement, 0, NULL);

    // Handle input in a separate thread so we can detect "exit" command immediately
    _beginthread(handleMarqueeInput, 0, NULL);

    // Wait here until marquee mode is exited
    while (in_marquee_mode) {
        Sleep(100); // Small delay to avoid CPU overuse
    }

    // Reset flags fully before returning to main menu
    in_marquee_mode = 0;
    in_main_menu = 1;
    displayMainMenu();  // Display main menu once marquee mode is fully exited
}

void handleMarqueeInput() {
    while (in_marquee_mode) {
        displayCommandInput(command);  // Display the command prompt in marquee mode
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') {  // Enter key pressed
                printf("\nCommand processed in MARQUEE_CONSOLE: %s\n", command);
                if (strcmp(command, "exit") == 0) {
                    in_marquee_mode = 0;  // Set flag to exit marquee mode
                    command[0] = '\0';    // Clear command buffer after exit
                } else {
                    printf("Only the 'exit' command is allowed in marquee mode.\n");
                    command[0] = '\0';    // Clear command buffer for re-entry
                }
            } else if (ch == '\b') {  // Handle backspace
                int len = strlen(command);
                if (len > 0) {
                    command[len - 1] = '\0';
                }
            } else {  // Append other characters to the command buffer
                int len = strlen(command);
                if (len < sizeof(command) - 1) {
                    command[len] = ch;
                    command[len + 1] = '\0';
                }
            }
        }
        Sleep(50);  // Short delay for smoother input handling
    }
}
