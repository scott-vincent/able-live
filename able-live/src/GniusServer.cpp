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
extern char _gniusData3[MaxGniusData];

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
        if (_gniusData == _gniusData1) {
            newGniusData = _gniusData2;
        }
        else if (_gniusData == _gniusData2) {
            newGniusData = _gniusData3;
        }
        else {
            newGniusData = _gniusData1;
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
                newGniusData[bytes] = '\0';
                _gniusData = newGniusData;
                //printf("Got gnius data: %s\n", _gniusData);
                //fflush(stdout);
                failures = 0;
            }
        }
        else {
            bytes = SOCKET_ERROR;
        }

        if (bytes == SOCKET_ERROR) {
            failures++;
            if (failures > 1) {
                *_gniusData = '\0';
                failures = 99;
            }
            else {
                printf("Wait for gnius data (%d)\n", failures);
                fflush(stdout);
            }
        }
    }

    serverCleanUp();
}
