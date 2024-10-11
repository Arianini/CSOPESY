#ifndef MODEL_H
#define MODEL_H

#include <windows.h>  // For Windows threading

#define MAX_PROCESSES 10
#define MAX_SCREENS 10
#define MAX_CORES 4
#define PRINT_COMMANDS 100

typedef struct {
    char name[50];
    int pid;
    int core_id;
    int instructions;
    int finished;
    char timestamp[50];
} Process;

typedef struct {
    char name[50];
    int id;
    int current_instruction;
    int total_instructions;
    char timestamp[50];
} Screen;

extern Process processes[MAX_PROCESSES];
extern Screen screens[MAX_SCREENS];
extern int process_count;
extern int current_process;
extern int screen_count;

void initializeScheduler();
DWORD WINAPI executeProcess(LPVOID arg);  // Windows thread function signature
void createScreen(char* name);
void resumeScreen(char* name);
void testScheduler();  // Add this declaration

#endif // MODEL_H
