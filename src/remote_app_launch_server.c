#include <stdio.h> // printf, fgets, scanf, sprintf
#include <string.h> // strcmp, strlen
#include <stdlib.h>

#include "network_manager/network_manager.h"

long send_request(int clientSocket, const char* requestCommand, const char* application) {
    char request[250];
    long request_len;

    if (requestCommand != NULL) {
        if (application != NULL) {
            printf("Sending: \"%s\", \"%s\"\n", requestCommand, application);
            sprintf(request, "%s %s", requestCommand, application);
        } else {
            printf("Sending: \"%s\"\n", requestCommand);
            sprintf(request, "%s", requestCommand);
        }
    } else {
        return -1;
    }
    request_len = send_message(clientSocket, request, strlen(request) + 1);
    if (request_len < 0) {
        return -1;
    }
    return request_len;
}

int main() {
    char input[300];
    char clientName[30];
    char requestCommand[30];
    char application[100];
    long requestLength = 0;

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

    requestLength = recv_message(clientSocket, clientName, sizeof(clientName));
    if (requestLength < 0) {
        printf("Client name: \"%s\"\nrequestLength = %ld\n", clientName, requestLength);
        return -1;
    }
    printf("Client name: \"%s\"\n! requestLength = %ld\n", clientName, requestLength);

    printf("Enter client name and command (run/kill):\n> ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%s %s", clientName, requestCommand);

    if (strcmp(requestCommand, "run") == 0) {
        sscanf(input, "%*s %*s %[^\n]", application);
        requestLength = send_request(clientSocket, requestCommand, application); // run app argv
    } else if (strcmp(requestCommand, "kill") == 0) {
        char *endPtr = application;
        sscanf(input, "%*s %*s %[^\n]", application);
        // a || (b && c)
        // Short-circuit evaluation: (a) is evaluated first. if a = 1, (b && c) are not evaluated
        if (strcmp(application, "all") == 0 || (strtol(application, &endPtr, 0) > 0 && endPtr != application)) {
            requestLength = send_request(clientSocket, requestCommand, application); // kill pid/all
        }
    } else {
        printf("Invalid command \"%s\"\n", requestCommand);
        return 111;
    }
    printf("! requestLength = %ld\n", requestLength);

    for (int i = 0; i < 100; ++i) {
        printf("%d\n", i);
        requestLength = recv_message(clientSocket, input, sizeof(input));
        if (requestLength > 0) {
            puts(input);
        } else {
            printf("! requestLength = %ld\n", requestLength);
            return -1;
        }
    }

    printf("! serverSocket = %d\n", serverSocket);
    printf("! clientSocket = %d\n", clientSocket);
    printf("! requestLength = %ld\n", requestLength);
    close_socket(clientSocket);
    close_host_socket(serverSocket);

    return 0;
}
