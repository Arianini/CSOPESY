#include <stdio.h>
#include <windows.h>   // For Sleep function and console functions
#include <string.h>    // For strlen
#include <stdlib.h>    // For system function

// Function to clear the screen
void clearScreen() {
    system("cls");
}

void header(){
    printf("**************************************\n");
    printf("* Displaying marquee console! *\n");
    printf("**************************************\n");
}

// Function to display the entire format from the image
void displayMarqueeWithDesign(const char *message, int x, int y, int consoleWidth, int consoleHeight) {
    clearScreen();
    header();
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

    printf("Enter a command for MARQUEE_CONSOLE: Notice the crude refresh. This is very dependent on your monitor!\n");
    printf("Command processed in MARQUEE_CONSOLE: This is a sample barebones immediate mode UI drawing.\n");
}

// Function to get the console's current width and height
void getConsoleSize(int *width, int *height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int cols, rows;

    // Get console screen buffer info
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    *width = cols;
    *height = rows;
}

int main() {
    const char message[] = "Hello world in marquee!";
    int length = strlen(message);
    
    int x = 0, y = 0;              // Starting position of the text (top-left corner)
    int xDirection = 1, yDirection = 1;  // 1 for moving right/down, -1 for moving left/up
    int consoleWidth, consoleHeight;

    while (1) {
        // Get the current console size
        getConsoleSize(&consoleWidth, &consoleHeight);

        // Display the text with the designed format
        displayMarqueeWithDesign(message, x, y, consoleWidth, consoleHeight);

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

        // Control the speed of the marquee
        Sleep(100); // 100 ms delay for smoother movement
    }

    return 0;
}
