//
// Created by Admin on Apr 4 2022.
//

// TODO: Check out http://www.di.uniba.it/~reti/LabProRete/Interazione(TCP)Client-Server_Portabile.pdf
#define PORT 5080
//#define NTENV

#if defined(WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char rcvbuf[1024], sndbuf[1024];

#if defined(WIN32)

struct CLIENT {
    SOCKET client_socket;
    struct sockaddr_in client_addr;
};

bool InitWinSock2_0() {
    WSADATA wsaData;
    WORD wVersion = MAKEWORD(2, 0);
    return !WSAStartup(wVersion, &wsaData);
}

bool WINAPI ClientThread(LPVOID lp_data) {
    struct CLIENT *client = (struct CLIENT *) lp_data;
    char buf[1024];
    int len;
    while (true) {
        if ((len = recv(client->client_socket, buf, sizeof(buf), 0)) > 0) {
            buf[len] = '\0';
            sprintf(sndbuf, "Client %s:%d: %s\n", inet_ntoa(client->client_addr.sin_addr), ntohs(client->client_addr.sin_port),
                    buf);
            puts(sndbuf);
            if (send(client->client_socket, sndbuf, sizeof(buf), 0) < 0) {
                fprintf(stderr, "Error in sending message");
                perror("");
            }
        } else {
            fprintf(stderr, "Error receiving data from %s: %d.\n", inet_ntoa(client->client_addr.sin_addr), WSAGetLastError());
            perror("");
            break;
        }
    }
    return true;
}

#else
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#ifndef SOCKET
#define SOCKET int
#endif
#endif


int main(int argc, const char *argv[]) {
    int child, len, option = 1, rd;
    SOCKET client_socket;
    struct sockaddr_in client_addr;

#if defined(WIN32)
    if (!InitWinSock2_0()) {
        fprintf(stderr, "Unable to Initialize Windows Socket environment %d\n", WSAGetLastError());
        perror("");
        return -1;
    } else printf("Initialized Windows Socket environment\n");
#endif
    // setbuf(stdout, NULL);
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
#if defined(WIN32)
        fprintf(stderr, "Unable to initialize server socket: %d.", WSAGetLastError());
        WSACleanup();
#else
        fprintf(stderr, "Unable to initialize server socket");
#endif
        perror("");
        return -1;
    } else printf("Socket successfully created.\n");
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == SOCKET_ERROR) {
#if defined(WIN32)
        fprintf(stderr, "setsockopt for SO_REUSEADDR failed with error: %d\n", WSAGetLastError());
#else
        fprintf(stderr, "setsockopt for SO_REUSEADDR failed.\n");
#endif
        perror("");
    } else printf("Set SO_REUSEADDR: ON\n");
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
#if defined(WIN32)
        fprintf(stderr, "Unable to bind to %s port %d: %d.\n", inet_ntoa(server_addr.sin_addr), PORT,
                WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
#else
        fprintf(stderr, "Unable to bind to %s port %d\n", inet_ntoa(server_addr.sin_addr), PORT);
        perror("");
        close(server_socket);
#endif
        perror("");
        return -1;
    } else printf("Bind successful.\n");

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
#if defined(WIN32)
        fprintf(stderr, "Unable to put server socket into listen state: %d.\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
#else
        fprintf(stderr, "Unable to put server socket into listen state.\n");
        close(server_socket);
#endif
        perror("");
        return -1;
    } else printf("Listening on %s:%d.\n", inet_ntoa(server_addr.sin_addr), PORT);
    socklen_t cli_addr_size = sizeof(client_addr);

    while (true) {
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, (socklen_t *) &cli_addr_size);
        if (client_socket == INVALID_SOCKET) {
#if defined(WIN32)
            fprintf(stderr, "Accept failed: %d.\n", WSAGetLastError());
#else
            fprintf(stderr, "Accept failed with %d\n", client_socket);
            perror("");
            return -1;
#endif
        } else {
#if defined(WIN32)
            HANDLE client_thread;
            struct CLIENT client;
            DWORD thread_id;

            client.client_addr = client_addr;
            client.client_socket = client_socket;
            printf("Client %s:%d joined the server.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            client_thread = CreateThread(NULL, 0,
                                         (LPTHREAD_START_ROUTINE) ClientThread,
                                         (LPVOID) &client, 0, &thread_id);
            if (client_thread == NULL)
                printf("Unable to create client thread %d.\n", WSAGetLastError());
            else CloseHandle(client_thread);
#else
            printf("Client %s:%d joined the server.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            child = fork();
            if (child < 0) {
                fprintf(stderr, "ERROR on fork\n");
                perror("");
                exit(-1);
            }
            printf("fork() returned %d.\n", child);
            if (child == 0) {
                close(server_socket);
                for (;;) {
                    rd = recv(client_socket, rcvbuf, sizeof(rcvbuf), 0);
                    if (rd < 0) {
                        fprintf(stderr, "Error in receiving message\n");
                        perror("");
                        break;
                    } else if (rd == 0) {
                        printf("Client %s disconnected\n", inet_ntoa(client_addr.sin_addr));
                        break;
                    } else {
                        sprintf(sndbuf, "Client %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
                                ntohs(client_addr.sin_port),
                                rcvbuf);
                        puts(sndbuf);
                        if (send(client_socket, sndbuf, sizeof(sndbuf), 0) < 0) {
                            fprintf(stderr, "Error in sending message\n");
                            perror("");
                        }
                    }
                    bzero(rcvbuf, sizeof(rcvbuf));
                }
                exit(0);
            }
#endif
        }
    }
#if defined(WIN32)
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif
    return 0;
}
