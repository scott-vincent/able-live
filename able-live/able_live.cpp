#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <string.h>
#endif
#include <iostream>
#include <thread>

const char* versionString = "v1.0.1";
bool _quit = false;
bool _serverQuit = false;
char _moduleFilename[256];

void ableServer();
void showChart();

int main(int argc, char **argv)
{
    printf("Able Live %s Copyright (c) 2024 Scott Vincent\n", versionString);

    strcpy(_moduleFilename, argv[0]);

    // Start the server thread
    std::thread serverThread(ableServer);

    // Yield so server can start
#ifdef _WINDOWS
    Sleep(100);
#else
    usleep(100000);
#endif

    showChart();

    if (!_quit || _serverQuit) {
        // delay so error message can be read
#ifdef _WINDOWS
        Sleep(2000);
#else
        usleep(2000000);
#endif
        _quit = true;
    }

    // Wait for server to exit
    serverThread.join();

    return 0;
}
