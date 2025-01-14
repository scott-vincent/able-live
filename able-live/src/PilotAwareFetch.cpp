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

// Variables
char _pilotAwareUrl[256];
char* _pilotAwareData = 0;
size_t _dataSize = 0;
bool _initialised = false;
CURL* _curl = NULL;


size_t curlWrite(void* inData, size_t size, size_t inSize, void* userdata)
{
    char** toData = (char**)userdata;
    const char* input = (const char*)inData;
    if (inSize == 0) {
        return 0;
    }

    if (!*toData) {
        *toData = (char*)malloc(inSize + 1);
    }
    else {
        #pragma warning(suppress: 6308)
        *toData = (char*)realloc(*toData, _dataSize + inSize + 1);
    }

    if (*toData) {
        memcpy(*toData + _dataSize, input, inSize);
        _dataSize += inSize;
        (*toData)[_dataSize] = '\0';
    }
    else {
        printf("Failed to allocate memory for curl response");
    }

    return inSize;
}

void fetchInit()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    _curl = curl_easy_init();
    if (!_curl) {
        curl_global_cleanup();
        printf("Failed to initialise curl\n");
        return;
    }

    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_pilotAwareData);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &curlWrite);

    // Get URL
    if (!readUrl(PilotAware_Url, _pilotAwareUrl)) {
        return;
    }

    _initialised = true;
}

void fetchCleanUp()
{
    if (_pilotAwareData) {
        free(_pilotAwareData);
    }

    if (_curl != NULL) {
        curl_easy_cleanup(_curl);
    }
    curl_global_cleanup();

    printf("PilotAware Fetch exiting\n");
    _pilotAwareQuit = true;
    _quit = true;
}

bool doRequest()
{
    if (!_initialised) {
        fprintf(stderr, "Request failed as not initialised");
        return false;
    }

    curl_easy_setopt(_curl, CURLOPT_URL, _pilotAwareUrl);

    if (_pilotAwareData) {
        free(_pilotAwareData);
    }

    _pilotAwareData = 0;
    _dataSize = 0;

    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Request failed (%s): %s\n", _pilotAwareUrl, curl_easy_strerror(res));
        return false;
    }

    return true;
}

void fetchRequest()
{
    if (!doRequest()) {
        if (_pilotAwareData) {
            free(_pilotAwareData);
        }
        _pilotAwareData = (char*)malloc(2);
        if (_pilotAwareData) {
            *_pilotAwareData = '\0';
        }
        return;
    }
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
        fetchRequest();
        milliSleep(500);
    }

    fetchCleanUp();
}
