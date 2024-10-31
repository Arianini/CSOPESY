#ifndef MODEL_H
#define MODEL_H

#include <windows.h>
#include <stdbool.h>

extern int NUM_CORES;
extern int MAX_PROCESSES;
extern char config_schedulerType[20];
extern int config_quantumCycles;
extern float config_batchProcessFreq;
extern int config_minIns;
extern int config_maxIns;
extern float config_delayPerExec;

#define MAX_SCREENS 10
#define MAX_SCHEDULER_PROCESSES 10

typedef struct {
    char name[50];
    int id;
    int current_instruction;
    int total_instructions;
    char timestamp[50];
    bool created_by_screen_s;
} Screen;

typedef struct {
    char name[50];
    int pid;
    int core_id;
    int last_core_id; 
    int instructions;
    int instructions_completed;
    int finished;
    char timestamp[50];
    int remaining_time;  // For remaining burst time
    int last_stop_time;  // For tracking the last stop time for time slice calculation
    int at;
    int bt;  
} Process;

extern Process* processes;
extern int process_count;
extern int scheduler_running;
extern int in_main_menu;
// Synchronization handles
extern HANDLE process_lock;
extern HANDLE stop_event;

// Function declarations
int initializeSchedulerFromFile();
void roundRobinScheduler(int core_id);
void fcfsScheduler(int core_id);
DWORD WINAPI executeProcess(LPVOID arg);
void stopScheduler();
void runSchedulerTest();
void initializeFCFSScheduler();
void initializeRoundRobinScheduler();
Process* getProcessByName(char* name);
Process* createNewProcess(char* name);

#endif // MODEL_H

