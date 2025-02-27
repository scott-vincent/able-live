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

// Data must be double buffered
char* _pilotAwareData;
char _pilotAwareData1[MaxPilotAwareData];
char _pilotAwareData2[MaxPilotAwareData];

char* _fr24Data;
char _fr24Data1[MaxFr24Data];
char _fr24Data2[MaxFr24Data];

char* _gniusData;
char _gniusData1[MaxGniusData];
char _gniusData2[MaxGniusData];
char* _mobileData;
char _mobileData1[MaxGniusData];
char _mobileData2[MaxGniusData];

void pilotAwareFetch();
void fr24Fetch();
void gniusServer();
void showChart();

int main(int argc, char **argv)
{
    printf("Able Live %s Copyright (c) 2025 Scott Vincent\n", versionString);
    fflush(stdout);

    strcpy(_moduleFilename, argv[0]);

    *_pilotAwareData1 = '\0';
    _pilotAwareData = _pilotAwareData1;

    *_fr24Data1 = '\0';
    _fr24Data = _fr24Data1;

    *_gniusData1 = '\0';
    _gniusData = _gniusData1;

    *_mobileData1 = '\0';
    _mobileData = _mobileData1;

    // Start the PilotAware thread
    std::thread pilotAwareThread(pilotAwareFetch);
    milliSleep(100);

    // Start the FR24 thread
    std::thread fr24Thread(fr24Fetch);
    milliSleep(100);

    // Start the G-NIUS server
    std::thread gniusThread(gniusServer);
    milliSleep(100);

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
