#ifdef _WINDOWS
#include <windows.h>
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKADDR sockaddr
#define closesocket close
#endif
#include <iostream>
#include "able_live.h"
#include "ChartFile.h"

const int Port = 53020;

// Externals
extern bool _quit;
extern bool _gniusQuit;

// Variables
extern char* _gniusData;
extern char _gniusData1[MaxGniusData];
extern char _gniusData2[MaxGniusData];
extern char* _mobileData;
extern char _mobileData1[MaxGniusData];
extern char _mobileData2[MaxGniusData];

static bool _initialised = false;
static SOCKET sockfd;
static sockaddr_in senderAddr;
static int addrSize = sizeof(senderAddr);
static int bytes;
time_t lastGniusData = 0;
time_t lastMobileData = 0;

static void serverInit()
{
#ifdef _WINDOWS
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("Failed to initialise Windows Sockets: %d\n", err);
        return;
    }
#endif

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Server failed to create UDP socket\n");
        fflush(stdout);
        return;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Port);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
#ifdef _WINDOWS
        int error = WSAGetLastError();
#else
        int error = errno;
#endif
        printf("Server failed to bind to localhost port %d: %ld\n", Port, error);
        fflush(stdout);
        return;
    }

    printf("G-NIUS Server listening on port %d\n", Port);
    fflush(stdout);

    _initialised = true;
}

static void serverCleanUp()
{
    closesocket(sockfd);

#ifdef _WINDOWS
    WSACleanup();
#endif

    printf("G-NIUS Server exiting\n");
    _gniusQuit = true;
    _quit = true;
}

void checkStale()
{
    time_t now;
    time(&now);

    if (_gniusData[0] != '\0' && now - lastGniusData > 3) {
        *_gniusData = '\0';
        lastGniusData = 0;
    }

    if (_mobileData[0] != '\0' && now - lastMobileData > 3) {
        *_mobileData = '\0';
        lastMobileData = 0;
    }
}

void gniusServer()
{
    serverInit();
    if (!_initialised) {
        serverCleanUp();
        return;
    }

    timeval timeout;

    while (!_quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Timeout needs to be set every time on Linux!
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        char* newGniusData;
        char* newMobileData;

        if (_gniusData == _gniusData1) {
            newGniusData = _gniusData2;
        }
        else {
            newGniusData = _gniusData1;
        }

        if (_mobileData == _mobileData1) {
            newMobileData = _mobileData2;
        }
        else {
            newMobileData = _mobileData1;
        }

        // Wait for Simulator to send data (non-blocking, 2 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, newGniusData, MaxGniusData, 0, (SOCKADDR*)&senderAddr, (socklen_t*)&addrSize);
            if (bytes == -1) {
#ifdef _WINDOWS
                int error = WSAGetLastError();
#else
                int error = errno;
#endif
                if (error == 10040) {
                    printf("Received more than %d bytes from %s (WSAError = %d)\n", MaxGniusData, inet_ntoa(senderAddr.sin_addr), error);
                }
                else {
                    printf("Received from %s but WSAError = %d\n", inet_ntoa(senderAddr.sin_addr), error);
                }
                fflush(stdout);
            }
            else {
                //printf("Received %d bytes of G-NIUS data from %s\n", bytes, inet_ntoa(senderAddr.sin_addr));
                time_t now;
                time(&now);

                newGniusData[bytes] = '\0';
                if (newGniusData[2] == '7') {
                    _gniusData = newGniusData;
                    if (lastGniusData <= 1) {
                        printf("Got Gnius data: %s\n", _gniusData);
                    }
                    lastGniusData = now;
                }
                else {
                    memcpy(newMobileData, newGniusData, bytes + 1);
                    _mobileData = newMobileData;
                    if (lastMobileData <= 1) {
                        printf("Got Mobile data: %s\n", _mobileData);
                    }
                    lastMobileData = now;
                }
                //fflush(stdout);
                checkStale();
            }
        }
        else {
            bytes = SOCKET_ERROR;
        }

        if (bytes == SOCKET_ERROR) {
            if (lastGniusData == 0) {
                printf("Wait for Gnius data\n");
                lastGniusData = 1;
            }
            if (lastMobileData == 0) {
                printf("Wait for Mobile data\n");
                lastMobileData = 1;
            }
            checkStale();
            milliSleep(500);
        }
    }

    serverCleanUp();
}
