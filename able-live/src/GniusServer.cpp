#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <iostream>
#include "able_live.h"
#include "ChartFile.h"

// Externals
extern bool _quit;
extern bool _gniusQuit;

// Variables
char _gniusHost[256];
char* _gniusData = 0;
static size_t _dataSize = 0;
static bool _initialised = false;


static void serverInit()
{
    // Get URL
    if (!readUrl(Gnius_Host, _gniusHost)) {
        return;
    }



    _initialised = true;
}

static void serverCleanUp()
{
    if (_gniusData) {
        free(_gniusData);
    }

    printf("G-NIUS Server exiting\n");
    _gniusQuit = true;
    _quit = true;
}

void gniusServer()
{
    serverInit();
    if (!_initialised) {
        serverCleanUp();
        return;
    }

    while (!_quit)
    {
        milliSleep(500);
    }

    serverCleanUp();
}
