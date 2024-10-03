#include <stdio.h>
#include <windows.h>   // For Sleep function and console functions
#include <string.h>    // For strlen
#include <stdlib.h>    // For system function

void clearScreen() {
    // Clears the terminal screen
    system("cls");
}

void displayMarquee(const char *message, int x, int y) {
    clearScreen();

    // Move cursor down (y-axis)
    for (int i = 0; i < y; i++) {
        printf("\n");
    }

    // Move cursor right (x-axis)
    for (int i = 0; i < x; i++) {
        printf(" ");
    }

    // Print the message
    printf("%s\n", message);
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

        // Display the text at the current position
        displayMarquee(message, x, y);

        // Update the position for the next frame (move diagonally)
        x += xDirection;
        y += yDirection;

        // Bounce off the right/left edges
        if (x + length >= consoleWidth || x <= 0) {
            xDirection = -xDirection;
        }

        // Bounce off the top/bottom edges
        if (y >= consoleHeight - 1 || y <= 0) {
            yDirection = -yDirection;
        }

        // Control the speed of the marquee
        Sleep(100); // 100 ms delay for smoother movement
    }

    return 0;
}
