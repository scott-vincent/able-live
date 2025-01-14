#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <string.h>
#endif
#include <iostream>
#include <thread>
#include "ChartFile.h"

const char* versionString = "v1.1.0";
bool _quit = false;
bool _pilotAwareQuit = false;
bool _fr24Quit = false;
bool _gniusQuit = false;
char _moduleFilename[256];

void pilotAwareFetch();
void fr24Fetch();
void gniusServer();
void showChart();

int main(int argc, char **argv)
{
    printf("Able Live %s Copyright (c) 2025 Scott Vincent\n", versionString);
    fflush(stdout);

    strcpy(_moduleFilename, argv[0]);

    // Start the PilotAware thread
    std::thread pilotAwareThread(pilotAwareFetch);
    milliSleep(125);

    // Start the FR24 thread
    std::thread fr24Thread(fr24Fetch);
    milliSleep(125);

    // Start the G-NIUS server
    std::thread gniusThread(gniusServer);
    milliSleep(125);

    showChart();

    if (!_quit || _pilotAwareQuit || _fr24Quit || _gniusQuit) {
        // delay so error message can be read
        milliSleep(2000);
        _quit = true;
    }

    // Wait for other threads to exit
    pilotAwareThread.join();
    fr24Thread.join();
    gniusThread.join();

    return 0;
}
