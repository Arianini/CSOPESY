#ifndef VIEW_H
#define VIEW_H

#include "model.h"

// Declare these as extern so they are accessible across files
extern int in_marquee_mode;
extern int in_main_menu;

void clearScreen();
void displayMainMenu();
void displayProcessLogs();
void displayHelp();
void displayScreen(Screen* screen);
void displayProcessSMI();
void displayMarquee();
void handleMarqueeInput();
void setConsoleColor(int color);
void resetConsoleColor();

#endif // VIEW_H