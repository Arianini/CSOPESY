#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <conio.h> // For _kbhit() and _getch()

#define REFRESH_RATE 100000 // Microseconds

void clearScreen() {
    // Clear screen using ANSI escape sequence
    printf("\033[H\033[J");
}

void displayStaticText() {
    // Display the static parts of the console
    printf("**************************************\n");
    printf("* Displaying a marquee console!      *\n");
    printf("**************************************\n\n");
}

void displayInputPrompt() {
    // Display the input prompt at the bottom
    printf("\n\n\n\nEnter a command for MARQUEE_CONSOLE: ");
    fflush(stdout);
}

void displayMarquee(int x, int y, const char* message) {
    // Clear the screen before displaying the static and moving parts
    clearScreen();

    // First, display the static text
    displayStaticText();

    // Move the cursor to the (y,x) position and display the moving text
    printf("\033[%d;%dH%s", y + 4, x, message); // Offset y to avoid static text area

    // Display the input prompt at the bottom
    displayInputPrompt();
}

void handleUserInput() {
    // Buffer for storing the user input
    char input[100];

    // Simulate user input and response
    fgets(input, sizeof(input), stdin);  // Get user input

    // Print the user's command and simulate an action
    printf("Entered command: %s. Doing something...\n", input);
    fflush(stdout);
}

// Get the size of the console window
void getWindowSize(int *width, int *height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    // Get the number of columns and rows in the current console window
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    *width = columns;
    *height = rows;
}

void bounceMarquee(const char* message) {
    int x = 0, y = 0; // Starting position
    int xDir = 1, yDir = 1; // Direction for movement
    int len = strlen(message);
    int width, height;

    while (1) {
        // Get the current window size
        getWindowSize(&width, &height);

        // Calculate the usable height for bouncing, avoiding the static areas
        int usableHeight = height - 10; // Leave extra buffer for input area
        int usableWidth = width;

        // Ensure the message fits within the usable area
        if (x + len >= usableWidth) xDir = -1;
        if (x <= 0) xDir = 1;
        if (y >= usableHeight) yDir = -1;
        if (y <= 0) yDir = 1;

        // Display the marquee at the current position
        displayMarquee(x, y, message);

        // Check if the user has entered something
        if (_kbhit()) {
            handleUserInput();  // If a key has been pressed, handle the input
        }

        // Move the position for the next iteration
        x += xDir;
        y += yDir;

        // Delay for smooth movement
        usleep(REFRESH_RATE);
    }
}

int main() {
    const char* message = "Hello world in marquee!";

    // Start bouncing the marquee
    bounceMarquee(message);

    return 0;
}

