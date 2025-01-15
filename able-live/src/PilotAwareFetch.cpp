#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <iostream>
#include <curl/curl.h>
#include "able_live.h"
#include "ChartFile.h"

// Externals
extern bool _quit;
extern bool _pilotAwareQuit;
extern char* _pilotAwareData;
extern char _pilotAwareData1[MaxPilotAwareData];
extern char _pilotAwareData2[MaxPilotAwareData];

// Variables
char _pilotAwareUrl[256];
static size_t _dataSize = 0;
static bool _initialised = false;
static CURL* _curl = NULL;
static int failures = 0;


static size_t curlWrite(void* inData, size_t size, size_t inSize, void* userdata)
{
    char* toData = (char*)userdata;
    const char* input = (const char*)inData;
    if (inSize == 0) {
        return 0;
    }

    int newSize = _dataSize + inSize;
    if (newSize < MaxPilotAwareData) {
        memcpy(toData + _dataSize, input, inSize);
        toData[newSize] = '\0';
        _dataSize = newSize;
    }
    else {
        printf("Buffer too small for pilotAware data: %d\n", newSize);
    }

    return inSize;
}

static void fetchInit()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    _curl = curl_easy_init();
    if (!_curl) {
        curl_global_cleanup();
        printf("Failed to initialise curl\n");
        return;
    }

    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &curlWrite);

    // Get URL
    if (!readUrl(PilotAware_Url, _pilotAwareUrl)) {
        return;
    }

    _initialised = true;
}

static void fetchCleanUp()
{
    if (_curl != NULL) {
        curl_easy_cleanup(_curl);
    }
    curl_global_cleanup();

    printf("PilotAware Fetch exiting\n");
    _pilotAwareQuit = true;
    _quit = true;
}

static bool doRequest()
{
    if (!_initialised) {
        fprintf(stderr, "Request failed as not initialised");
        return false;
    }

    char* newPilotAwareData;
    if (_pilotAwareData == _pilotAwareData1) {
        newPilotAwareData = _pilotAwareData2;
    }
    else {
        newPilotAwareData = _pilotAwareData1;
    }

    curl_easy_setopt(_curl, CURLOPT_URL, _pilotAwareUrl);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, newPilotAwareData);

    _dataSize = 0;
    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Fetch PilotAware data failed: %s\n", curl_easy_strerror(res));
        return false;
    }

    _pilotAwareData = newPilotAwareData;
    return true;
}

void pilotAwareFetch()
{
    fetchInit();
    if (!_initialised) {
        fetchCleanUp();
        return;
    }

    while (!_quit)
    {
        if (doRequest()) {
            //printf("Got pilotAware data (%lld bytes)\n", strlen(_pilotAwareData));
            failures = 0;
        }
        else {
            failures++;
            if (failures > 3) {
                *_pilotAwareData = '\0';
                failures = 99;
            }
            else {
                printf("Wait for pilotAware data (%d)\n", failures);
            }
        }

        milliSleep(1000);
    }

    fetchCleanUp();
}
