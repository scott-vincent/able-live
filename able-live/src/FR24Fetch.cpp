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

// Variables
char _ableDisplayUrl[256];
char* _fr24Data = 0;
static size_t _dataSize = 0;
static bool _initialised = false;
static CURL* _curl = NULL;


static size_t curlWrite(void* inData, size_t size, size_t inSize, void* userdata)
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

static void fetchInit()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    _curl = curl_easy_init();
    if (!_curl) {
        curl_global_cleanup();
        printf("Failed to initialise curl\n");
        return;
    }

    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_fr24Data);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &curlWrite);

    // Get URL
    if (!readUrl(AbleDisplay_Url, _ableDisplayUrl)) {
        return;
    }

    _initialised = true;
}

static void fetchCleanUp()
{
    if (_fr24Data) {
        free(_fr24Data);
    }

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

    curl_easy_setopt(_curl, CURLOPT_URL, _ableDisplayUrl);

    if (_fr24Data) {
        free(_fr24Data);
    }

    _fr24Data = 0;
    _dataSize = 0;

    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Request failed (%s): %s\n", _ableDisplayUrl, curl_easy_strerror(res));
        return false;
    }

    return true;
}

static void fetchRequest()
{
    if (!doRequest()) {
        if (_fr24Data) {
            free(_fr24Data);
        }
        _fr24Data = (char*)malloc(2);
        if (_fr24Data) {
            *_fr24Data = '\0';
        }
        return;
    }
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
        fetchRequest();
        milliSleep(500);
    }

    fetchCleanUp();
}
