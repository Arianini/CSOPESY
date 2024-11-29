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
extern int stop_generating;  // Flag to control process generation

#define MAX_SCREENS 10
#define MAX_SCHEDULER_PROCESSES 10
#define INITIAL_MAX_PROCESSES 10  // Initial allocation size
#define RESIZE_THRESHOLD 0.8

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
    int allocated_memory;  // Starting address of allocated memory
    int memory_size;       // Memory size allocated for this process
} Process;

typedef struct {
    int start_addr;
    int size;
    int pid; // Process ID using this memory
} MemoryBlock;

extern Process* processes;
extern Process* current_screen_process;
extern int process_count;
extern int scheduler_running;
extern int in_main_menu;
// Synchronization handles
extern HANDLE process_lock;
extern HANDLE stop_event;

// Function declarations
int initializeSchedulerFromFile();
int getAvailableCore();
void roundRobinScheduler(int core_id);
void fcfsScheduler(int core_id);
DWORD WINAPI executeProcess(LPVOID arg);
DWORD WINAPI generateProcesses(LPVOID lpParam);
void stopScheduler();
void runSchedulerTest();
void initializeFCFSScheduler();
void initializeRoundRobinScheduler();
void resizeProcessesArray();
Process* getProcessByName(char* name);
Process* createNewProcess(char* name);

Process* getNextProcess(int core_id);
void getTimestamp(char* buffer, size_t bufferSize);
int allocateMemory(int pid, int required_size);
void deallocateMemory(int pid);
int getExternalFragmentation();
void recordMemorySnapshot(int cycle);
void freeMemory(int pid);
int calculateExternalFragmentation();
void displayMainMenuProcessSMI();
void startBackgroundTasks();
void retryMemoryAllocation();
#endif // MODEL_H

