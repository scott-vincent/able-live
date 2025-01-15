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
extern bool _fr24Quit;
extern char* _fr24Data;
extern char _fr24Data1[MaxFr24Data];
extern char _fr24Data2[MaxFr24Data];

// Variables
char _ableDisplayUrl[256];
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
    if (newSize < MaxFr24Data) {
        memcpy(toData + _dataSize, input, inSize);
        toData[newSize] = '\0';
        _dataSize = newSize;
    }
    else {
        printf("Buffer too small for fr24 data: %d\n", newSize);
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
    if (!readUrl(AbleDisplay_Url, _ableDisplayUrl)) {
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

    printf("FR24 Fetch exiting\n");
    _fr24Quit = true;
    _quit = true;
}

static bool doRequest()
{
    if (!_initialised) {
        fprintf(stderr, "Request failed as not initialised");
        return false;
    }

    char* newFr24Data;
    if (_fr24Data == _fr24Data1) {
        newFr24Data = _fr24Data2;
    }
    else {
        newFr24Data = _fr24Data1;
    }

    curl_easy_setopt(_curl, CURLOPT_URL, _ableDisplayUrl);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, newFr24Data);

    _dataSize = 0;
    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Fetch FR24 data failed: %s\n", curl_easy_strerror(res));
        return false;
    }

    _fr24Data = newFr24Data;
    return true;
}

void fr24Fetch()
{
    fetchInit();
    if (!_initialised) {
        fetchCleanUp();
        return;
    }

    while (!_quit)
    {
        if (doRequest()) {
            //printf("Got fr24 data (%lld bytes)\n", strlen(_fr24Data));
            failures = 0;
        }
        else {
            failures++;
            if (failures > 3) {
                *_fr24Data = '\0';
                failures = 99;
            }
            else {
                printf("Wait for fr24 data (%d)\n", failures);
            }
        }

        milliSleep(1000);
    }

    fetchCleanUp();
}
