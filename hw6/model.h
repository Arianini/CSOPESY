#ifndef MODEL_H
#define MODEL_H

#include <windows.h>

#define MAX_PROCESSES 10
#define MAX_CORES 4
#define MAX_SCREENS 10
#define PRINT_COMMANDS 100

typedef struct {
    char name[50];
    int pid;
    int core_id;
    char timestamp[50];
    int instructions;
    int finished;
} Process;

typedef struct {
    char name[50];
} Screen;

extern Process processes[MAX_PROCESSES];
extern Screen screens[MAX_SCREENS];
extern int process_count;
extern int current_process;
extern int screen_count;
extern HANDLE process_lock;

void initializeScheduler();
void testScheduler();
DWORD WINAPI executeProcess(LPVOID arg);
void createScreen(char* name);
void resumeScreen(char* name);
void listScreens();

#endif
