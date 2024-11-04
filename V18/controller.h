#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h" 

void handleCommand(char* command);
void runSchedulerTest();
void reportProgress();
void createScreen(char* name);
void resumeScreen(char* name);
void generateUtilizationReport();
void displayHelp();
Screen* getScreenByName(char* name);

#endif // CONTROLLER_H

