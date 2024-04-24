#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "network_manager.h"

int create_socket() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPPROTO_TCP or 0 ?
    return serverSocket;
}

int bind_socket(int serverSocket, uint16_t port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(serverSocket);
        return -1;
    }
    return 0;
}

int listen_port(int serverSocket, int n) {
    if (n < 1 || n > SOMAXCONN) {
        return -1;
    }
    if (listen(serverSocket, n) == -1) {
        close(serverSocket);
        return -1;
    }
    return 0;
}

int connect_to_server(int clientSocket, const char* ipAddr, uint16_t port) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
    serverAddr.sin_port = htons(port);
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(clientSocket);
        return -1;
    }
    return 0;
}

int select_clients(int serverSocket) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(serverSocket, &readfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    return select(serverSocket + 1, &readfds, NULL, NULL, &tv);
}

int accept_client(int serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if (clientSocket == -1) {
        close(serverSocket);
        return -1;
    }
    return clientSocket;
}

long send_message(int clientSocket, const char* message, size_t length) {
    return send(clientSocket, message, length, 0);
}

long recv_message(int clientSocket, char* buffer, size_t length) {
    return recv(clientSocket, buffer, length, 0);
}

int close_socket(int clientSocket) {
    return close(clientSocket);
}

int close_host_socket(int hostSocket) {
    return close(hostSocket);
}
