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

// Externals
extern bool _quit;
extern bool _serverQuit;

// Variables
char _url[256];
bool _fr24Request = false;
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

bool ping(char* url)
{
    // Extract ip address
    char ip[256];

    char* pos = strchr(url, '/');
    if (!pos) {
        return false;
    }
    pos++;

    if (*pos != '/') {
        return false;
    }
    pos++;

    char* atPos = strchr(pos, '@');
    if (atPos) {
        pos = atPos + 1;
    }

    strcpy(ip, pos);

    pos = strchr(ip, '/');
    if (!pos) {
        return false;
    }
    *pos = '\0';

    pos = strchr(ip, ':');
    if (pos) {
        *pos = '\0';
    }

    char command[64];
#ifdef _WINDOWS
    sprintf(command, "ping -4 -n 1 -w 1000 %s", ip);
#else
    sprintf(command, "ping -4 -c 1 -w 1 %s", ip);
#endif

    int res = system(command);
    return res == 0;
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

    // Get working URL
    FILE* inf = fopen(AbleLiveUrls, "r");
    if (!inf) {
        printf("File not found: %s\n", AbleLiveUrls);
        return;
    }

    char urlLocal[256];
    char urlInternet[256];
    *urlInternet = '\0';

    int size = fread(urlLocal, 1, 256, inf);
    urlLocal[size] = '\0';

    char* pos = strchr(urlLocal, '\n');
    if (pos) {
        *pos = '\0';
        strcpy(urlInternet, pos + 1);
        pos = strchr(urlInternet, '\n');
        if (pos) {
            *pos = '\0';
        }

        pos = strchr(urlInternet, '\r');
        if (pos) {
            *pos = '\0';
        }
    }

    pos = strchr(urlLocal, '\r');
    if (pos) {
        *pos = '\0';
    }

    fclose(inf);

    if (*urlInternet == '\0') {
        strcpy(_url, urlLocal);
    }
    else if (ping(urlLocal)) {
        printf("Found local PilotAware\n");
        strcpy(_url, urlLocal);
    }
    else {
        printf("Using internet PilotAware as local not found\n");
        strcpy(_url, urlInternet);
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
        curl_easy_setopt(_curl, CURLOPT_URL, FR24_Url);
    }
    else {
        curl_easy_setopt(_curl, CURLOPT_URL, _url);
    }

    if (_ableData) {
        free(_ableData);
    }

    _ableData = 0;
    _dataSize = 0;

    CURLcode res = curl_easy_perform(_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
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
