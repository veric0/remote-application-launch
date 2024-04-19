#include <stdio.h>
#include <string.h>

#include "network_manager/network_manager.h"

int main() {
    printf("Hello, Server!\n");
    char message[] = "Hello from server";

    int serverSocket = create_socket();
    if (serverSocket == -1) {
        printf("Failed to create socket.\n");
        return 1;
    }
    if (bind_socket(serverSocket, 8881) == -1) {
        printf("Bind failed.\n");
        return 2;
    }
    if (listen_port(serverSocket) == -1) {
        printf("Listen failed.\n");
        return 3;
    }
    int clientSocket = accept_client(serverSocket);
    if (clientSocket == -1) {
        printf("Accept failed.\n");
        return 5;
    }
    int request_len = send_message(clientSocket, message, strlen(message));
    printf("serverSocket = %d\n", serverSocket);
    printf("clientSocket = %d\n", clientSocket);
    printf("request_len = %d\n", request_len);
    close_socket(clientSocket);
    close_host_socket(serverSocket);

    return 0;
}
