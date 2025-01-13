#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <iostream>
#include <thread>
#include <curl/curl.h>
#include "able_live.h"
#include "AbleClient.h"
#include "ChartFile.h"

// Externals
extern bool _quit;
extern bool _serverQuit;

// Variables
char _pilotAwareUrl[256];
char _ableDisplayUrl[256];
char _gniusLaptopUrl[256];
bool _fr24Request = false;
bool _gniusRequest = false;
bool _ableRequest = false;
bool _ableResponse = false;
char* _ableData = 0;
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

void serverInit()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    _curl = curl_easy_init();
    if (!_curl) {
        curl_global_cleanup();
        printf("Failed to initialise curl\n");
        return;
    }

    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_ableData);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &curlWrite);

    // Get URLs
    if (!readUrl(PilotAware_Url, _pilotAwareUrl)) {
        return;
    }

    if (!readUrl(AbleDisplay_Url, _ableDisplayUrl)) {
        return;
    }

    if (!readUrl(GniusLaptop_Url, _gniusLaptopUrl)) {
        return;
    }

    _initialised = true;
}

void serverCleanUp()
{
    if (_ableData) {
        free(_ableData);
    }

    if (_curl != NULL) {
        curl_easy_cleanup(_curl);
    }
    curl_global_cleanup();

    printf("Exiting\n");
    _serverQuit = true;
    _quit = true;
}

bool doRequest()
{
    if (!_initialised) {
        fprintf(stderr, "Request failed as not initialised");
        return false;
    }

    if (_fr24Request) {
        curl_easy_setopt(_curl, CURLOPT_URL, _ableDisplayUrl);
    }
    else if (_gniusRequest) {
        curl_easy_setopt(_curl, CURLOPT_URL, _gniusLaptopUrl);
    }
    else {
        curl_easy_setopt(_curl, CURLOPT_URL, _pilotAwareUrl);
    }

    if (_ableData) {
        free(_ableData);
    }

    _ableData = 0;
    _dataSize = 0;

    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        char url[256];
        if (_fr24Request) {
            strcpy(url, _ableDisplayUrl);
        }
        else if (_gniusRequest) {
            strcpy(url, _gniusLaptopUrl);
        }
        else {
            strcpy(url, _pilotAwareUrl);
        }
        fprintf(stderr, "Request failed (%s): %s\n", url, curl_easy_strerror(res));
        return false;
    }

    return true;
}

void ableRequest()
{
    if (!doRequest()) {
        if (_ableData) {
            free(_ableData);
        }
        _ableData = (char*)malloc(2);
        if (_ableData) {
            *_ableData = '\0';
        }
        _ableResponse = true;
        return;
    }

    _ableResponse = true;
}

void ableServer()
{
    serverInit();
    if (!_initialised) {
        serverCleanUp();
        return;
    }

    while (!_quit)
    {
        if (_ableRequest) {
            ableRequest();
            _ableRequest = false;
        }

#ifdef _WINDOWS
        Sleep(50);
#else
        usleep(50000);
#endif
    }

    serverCleanUp();
}
