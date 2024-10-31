#ifndef CONTROLLER_H
#define CONTROLLER_H

void handleCommand(char* command);
void runSchedulerTest();
void reportProgress();
void createScreen(char* name);
void resumeScreen(char* name);
void generateUtilizationReport();
void displayHelp();

#endif // CONTROLLER_H

