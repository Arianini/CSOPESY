#include <stdio.h>
#include <windows.h>   // For Sleep function and console functions
#include <string.h>    // For strlen
#include <stdlib.h>    // For system function
#include <conio.h>     // For _kbhit() and _getch()

// Function to clear a specific line in the console
void clearLine(int y) {
    COORD coord;
    coord.X = 0;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    for (int i = 0; i < 80; i++) printf(" "); // Clear line
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord); // Reset cursor
}

// Function to clear the screen
void clearScreen() {
    system("cls");
}

// Function to display the entire format from the image
void displayMarqueeWithDesign(const char *message, int x, int y, int consoleWidth, int consoleHeight, const char* command) {
    clearScreen();
    printf("**************************************\n");
    printf("* Displaying marquee console! *\n");
    printf("**************************************\n");

    // Print empty lines to position the marquee in y-axis
    for (int i = 0; i < y; i++) {
        printf("\n");
    }

    // Print spaces to position the marquee in x-axis
    for (int i = 0; i < x; i++) {
        printf(" ");
    }

    // Print the marquee text
    printf("%s\n", message);

    // Print the static bottom section
    for (int i = 0; i < consoleHeight - y - 7; i++) {
        printf("\n");
    }

    printf("Enter a command for MARQUEE_CONSOLE:");
}

// Function to get the console's current width and height
void getConsoleSize(int *width, int *height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

// Function to simulate a heavy workload
void simulateHeavyWorkload() {
    for (int i = 0; i < 50000; i++) {
        (void)(i * i); // Simulate CPU work without assigning the result
    }
}

int main() {
    const char message[] = "Hello world in marquee!";
    int length = strlen(message);
    
    int x = 0, y = 0;              // Starting position of the text (top-left corner)
    int xDirection = 1, yDirection = 1;  // 1 for moving right/down, -1 for moving left/up
    int consoleWidth, consoleHeight;
    
    // Variable for user input
    char command[100] = "";

    // Get initial console size
    getConsoleSize(&consoleWidth, &consoleHeight);

    while (1) {
        // Simulate heavy workload
        simulateHeavyWorkload();

        // Get the current console size
        getConsoleSize(&consoleWidth, &consoleHeight);

        // Display the marquee
        displayMarqueeWithDesign(message, x, y, consoleWidth, consoleHeight, command);

        // Check for bounce on right or left edges and reverse direction if needed
        if (x + length >= consoleWidth) {
            xDirection = -1;  // Move left if hitting the right edge
            x = consoleWidth - length - 1;  // Prevent moving past the boundary
        } else if (x <= 0) {
            xDirection = 1;   // Move right if hitting the left edge
            x = 0;            // Prevent moving past the boundary
        }

        // Check for bounce on bottom or top edges and reverse direction if needed
        if (y >= consoleHeight - 7) {  // Adjusted for frame and bottom text
            yDirection = -1;  // Move up if hitting the bottom edge
            y = consoleHeight - 7;  // Prevent moving past the boundary
        } else if (y <= 0) {
            yDirection = 1;   // Move down if hitting the top edge
            y = 0;            // Prevent moving past the boundary
        }

        // Update the position for the next frame (move diagonally)
        x += xDirection;
        y += yDirection;

        // Check if a key is pressed
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') { // Enter key
                // When Enter is pressed, process the command
                printf("\nCommand processed in MARQUEE_CONSOLE: %s\n", command);
                command[0] = '\0';  // Reset the command buffer
            } else if (ch == '\b') { // Backspace key
                // Handle backspace
                int len = strlen(command);
                if (len > 0) {
                    command[len - 1] = '\0';  // Remove the last character
                }
            } else {
                // Add typed character to command buffer
                int len = strlen(command);
                if (len < sizeof(command) - 1) {
                    command[len] = ch;
                    command[len + 1] = '\0';
                }
            }
        }

        // Control the speed of the marquee
        Sleep(100); // Adjust for smoother/slower movement (current: 100ms delay)
    }

    return 0;
}
