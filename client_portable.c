//
// Created by Admin on Apr 4 2022.
//

// TODO: Check out http://www.di.uniba.it/~reti/LabProRete/Interazione(TCP)Client-Server_Portabile.pdf

#define PORT 5080

//#define NTENV

#if defined(WIN32)

#include <winsock2.h>

#else

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[]) {
#if defined(WIN32)
    WSADATA wsaData;
#endif
    SOCKET client_socket;
    struct sockaddr_in server_addr, sender;
    int conn, total_bytes_rcvd = 0, bytes_rcvd;
    char snbuf[1024], rcvbuf[1024];

    int bytes_sent, nlen;

#if defined(WIN32)
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    printf("Client: Winsock DLL status is %s.\n", wsaData.szSystemStatus);
#endif

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client_socket == INVALID_SOCKET) {
#if defined(WIN32)
        fprintf(stderr, "Client: socket() failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
#else
        fprintf(stderr, "Client: socket() failed.\n");
#endif
        return -1;
    } else
        printf("Client: socket() is OK!\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    conn = connect(client_socket, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
    if (conn < 0) {
#if defined(WIN32)
        fprintf(stderr, "Client: connect() failed. Error code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
#else
        fprintf(stderr, "Client: connect() failed.\n");
#endif
        return -1;
    } else {
        printf("Client: connect() is OK, got connected...\n");
        printf("Client: Ready for sending and/or receiving data...\n");

        getsockname(client_socket, (struct sockaddr *) &server_addr, (int *) sizeof(server_addr));
        printf("Client: Receiver IP(s) used: %s\n", inet_ntoa(server_addr.sin_addr));
        printf("Client: Receiver port used: %d\n", htons(server_addr.sin_port));

        while (true) {
            printf("Enter string: ");
            fflush(stdout);
            gets(snbuf);
            fflush(stdin);
            if (strcasecmp(snbuf, ".quit") == 0) {
                send(client_socket, NULL, sizeof(NULL), 0);
                break;
            }
            bytes_sent = send(client_socket, snbuf, strlen(snbuf), 0);
            if (bytes_sent == SOCKET_ERROR) {
#if defined(WIN32)
                fprintf(stderr, "Client: send() error %d.\n", WSAGetLastError());
#else
                fprintf(stderr, "Client: send() error.\n");
#endif
            } else {
                printf("Client: send() is OK - bytes sent: %d\n", bytes_sent);
                memset(&sender, 0, sizeof(sender));
                nlen = sizeof(sender);

                getsockname(client_socket, (struct sockaddr *) &sender, &nlen);
                printf("Client: Sender IP(s) used: %s\n", inet_ntoa(sender.sin_addr));
                printf("Client: Sender port used: %d\n", htons(sender.sin_port));
                printf("Client: Those bytes represent: \"%s\"\n", snbuf);
            }

            while (total_bytes_rcvd < sizeof(snbuf)) {
                if ((bytes_rcvd = recv(client_socket, rcvbuf, sizeof(rcvbuf) - 1, 0)) <= 0) {
                    fprintf(stderr, "recv() failed or connection closed prematurely");
                    perror("");
                    goto closing;
                }
                total_bytes_rcvd += bytes_rcvd; // Keep tally of total bytes
                rcvbuf[bytes_rcvd] = '\0'; // Add \0 so printf knows where to stop
                printf("%s", rcvbuf); // Print the echo buffer
            }
        }
        closing:
#if defined(WIN32)
        if (shutdown(client_socket, SD_SEND) != 0) {
            fprintf(stderr, "Client: Well, there is something wrong with the shutdown(). The error code: %d\n",
                   WSAGetLastError());
            perror("");
        } else
            printf("Client: shutdown() looks OK...\n");
        if (closesocket(client_socket) != 0) {
            fprintf(stderr, "Client: Cannot close \"SendingSocket\" socket. Error code: %d\n", WSAGetLastError());
            perror("");
        } else
            printf("Client: Closing \"SendingSocket\" socket...\n");
        if (WSACleanup() != 0) {
            fprintf(stderr, "Client: WSACleanup() failed!...\n");
            perror("");
        } else
            printf("Client: WSACleanup() is OK...\n");
#else
        if (close(client_socket) != 0) {
            fprintf(stderr, "Client: Cannot close \"SendingSocket\" socket.\n");
            perror("");
        } else
            printf("Client: Closing \"SendingSocket\" socket...\n");
#endif
    }
    return 0;
}