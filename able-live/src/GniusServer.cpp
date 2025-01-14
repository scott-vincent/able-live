#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <iostream>
#include "able_live.h"
#include "ChartFile.h"

const int Port = 53020;
const int MaxDataSize = 1024;

// Externals
extern bool _quit;
extern bool _gniusQuit;

// Variables
char* _gniusData = 0;
static size_t _dataSize = 0;
static bool _initialised = false;
static SOCKET sockfd;
static sockaddr_in senderAddr;
static int addrSize = sizeof(senderAddr);
static int bytes;
static int failures = 0;

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
        return;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Port);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Server failed to bind to localhost port %d: %ld\n", Port, WSAGetLastError());
        return;
    }

    printf("G-NIUS Server listening on port %d\n", Port);

    _gniusData = (char*)malloc(MaxDataSize);
    _initialised = true;
}

static void serverCleanUp()
{
    if (_gniusData) {
        free(_gniusData);
    }

    closesocket(sockfd);

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

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    while (!_quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Wait for Simulator to send data (non-blocking, 0.5 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, (char*)&_gniusData, MaxDataSize, 0, (SOCKADDR*)&senderAddr, &addrSize);
            if (bytes == -1) {
                int error = WSAGetLastError();
                if (error == 10040) {
                    printf("Received more than %d bytes from %s (WSAError = %d)\n", MaxDataSize, inet_ntoa(senderAddr.sin_addr), error);
                }
                else {
                    printf("Received from %s but WSAError = %d\n", inet_ntoa(senderAddr.sin_addr), error);
                }
            }
            else {
                failures = 0;
            }
        }
        else {
            bytes = SOCKET_ERROR;
        }

        if (bytes == SOCKET_ERROR) {
            failures++;
            if (failures > 2) {
                *_gniusData = '\0';
                failures = 99;
            }
        }
    }

    serverCleanUp();
}
