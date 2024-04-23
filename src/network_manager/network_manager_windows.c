//#include <winsock2.h>
//#pragma comment(lib, "ws2_32.lib")

#include "network_manager.h"

#include <stddef.h> // size_t

int create_socket() {
//    WSAStartup();
//    socket();
    return -1;
}

int bind_socket(int serverSocket, uint16_t port) {
//    bind();
    return -1;
}

int listen_port(int serverSocket) {
//    listen();
    return -1;
}

int connect_to_server(int clientSocket, const char* ipAddr, uint16_t port) {
//    connect();
    return -1;
}

int accept_client(int serverSocket) {
//    accept();
    return -1;
}

long send_message(int clientSocket, const char* message, size_t length) {
//    send();
    return -1;
}

long recv_message(int clientSocket, char* buffer, size_t length) {
//    recv();
    return -1;
}

int close_socket(int clientSocket) {
//    closesocket();
//    WSACleanup();
    return -1;
}
