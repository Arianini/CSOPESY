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
    // Display dummy GPU information
    printf("+------------------------------------------------------------------------------------+\n");
    printf("| NVIDIA-SMI 551.86               Driver Version: 551.86       CUDA Version: 12.4    |\n");
    printf("|--------------------------------------+----------------------+----------------------+\n");
    printf("| GPU  Name                 TCC/WDDM   | Bus-Id        Disp.A | Volatile Uncorr. ECC |\n");
    printf("| Fan  Temp  Perf        Pwr:Usage/Cap | Memory-Usage         | GPU-Util  Compute M. |\n");
    printf("|                                      |                      |               MIG M. |\n");
    printf("|======================================+======================+======================|\n");
    printf("|  0   NVIDIA GeForce GTX 1080    WDDM | 00000000:26:00.0  On |                  N/A |\n");
    printf("| 28%%   37C    P8          11W / 180W |   701MiB /   8192MiB |       0%     Default |\n");
    printf("|                                      |                      |                  N/A |\n");
    printf("+--------------------------------------+----------------------+----------------------+\n\n");
    printf("+------------------------------------------------------------------------------------+\n");
    printf("| Processes:                                                                         |\n");
    printf("| GPU   GI   CI     PID   Type   Process name                             GPU Memory |\n");
    printf("|       ID   ID                                                           Usage      |\n");
    printf("|====================================================================================|\n");
    printf("|   0    N/A  N/A  1368  C+G    C:\\Windows\\System32\\dwm.exe              N/A      |\n");
    printf("|   0    N/A  N/A  2116  C+G    C:\\Windows\\explorer.exe                   N/A      |\n");
    printf("|   0    N/A  N/A  4224  C+G    C:\\Program Files\\SomeApp\\app.exe         N/A      |\n");
    printf("|   0    N/A  N/A  56840 C+G    C:\\Games\\GameBar\\widgets.exe             N/A      |\n");
    printf("|   0    N/A  N/A  6676  C+G    C:\\Windows\\System32\\SearchHost.exe       N/A      |\n");
    printf("+------------------------------------------------------------------------------------+\n");
}

void main(void) {
    // Optional: Set console window size for better display
    setConsoleSize();
    
    // Display NVIDIA-SMI Layout
    displayNvidiaSmiLayout();
}
