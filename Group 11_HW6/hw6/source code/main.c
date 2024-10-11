#include <stdio.h>
#include "view.h"
#include "controller.h"

int main() {
    char command[100];
    displayMainMenu();
    while (1) {
        printf("\nEnter a command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        handleCommand(command);
    }
    return 0;
}
