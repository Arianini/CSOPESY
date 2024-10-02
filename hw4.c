#include <stdio.h>
#include <windows.h>

void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setConsoleSize(void) {
    SMALL_RECT windowSize = {0, 0, 90, 30};  // {Left, Top, Right, Bottom}
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &windowSize);

    COORD bufferSize = {90, 1000};  
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), bufferSize);
}

void displayNvidiaSmiLayout(void) {
    printf("+---------------------------------------------------------------------------------+\n");
    printf("| NVIDIA-SMI 551.86                Driver Version: 551.86    CUDA Version: 12.4   |\n");
    printf("|-------------------------------+----------------------+--------------------------|\n");
    printf("| GPU  Name        Persistence-M | Bus-Id        Disp.A | Volatile Uncorr. ECC    |\n");
    printf("| Fan  Temp  Perf  Pwr:Usage/Cap | Memory-Usage         | GPU-Util  Compute M.    |\n");
    printf("|                               |                      | MIG M.                   |\n");
    printf("|===============================+======================+==========================|\n");
    printf("|   0  GeForce GTX 1080    On    | 00000000:26:00.0 Off | N/A                     |\n");
    printf("| 28%%     37C    P8    11W / 180W  |  701MiB / 8192MiB    | 0%%       Default      |\n");
    printf("+-------------------------------+----------------------+--------------------------+\n");

    printf("| Processes:                                                       GPU Memory     |\n");
    printf("|  GPU   PID  Type    Process name                                  Usage         |\n");
    printf("|=================================================================================|\n");
    printf("|    0  1368  C+G     C:\\Windows\\System32\\dwm.exe                     N/A         |\n");
    printf("|    0  2116  C+G     C:\\Windows\\explorer.exe                         N/A         |\n");
    printf("|    0  4244  C+G     C:\\Program Files\\SomeApp\\app.exe                N/A         |\n");
    printf("|    0  5684  C+G     C:\\Games\\GameBar\\widgets.exe                    N/A         |\n");
    printf("|    0  6672  C+G     C:\\Windows\\System32\\SearchHost.exe              N/A         |\n");
    printf("+---------------------------------------------------------------------------------+\n");
}

void main(void) {
    // Optional: Set console window size for better display
    setConsoleSize();
    
    // Display NVIDIA-SMI Layout
    displayNvidiaSmiLayout();
}