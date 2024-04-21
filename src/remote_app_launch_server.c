#include <stdio.h> // printf, fgets, scanf, sprintf
#include <string.h> // strcmp, strlen

#include "network_manager/network_manager.h"

int send_request(int clientSocket, char* requestCommand, char* application, char* arguments) {
    char request[250];
    int request_len = 0;

    if (requestCommand != NULL) {
        if (application != NULL) {
            if (arguments != NULL) {
                printf("Sending: \"%s\", \"%s\", \"%s\"\n", requestCommand, application, arguments);
                sprintf(request, "%s %s %s", requestCommand, application, arguments);
            } else {
                printf("Sending: \"%s\", \"%s\"\n", requestCommand, application);
                sprintf(request, "%s %s", requestCommand, application);
            }
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
    char arguments[100];
    int request_len = 0;

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

    printf("Enter client name and command (run/kill):\n> ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%s %s", clientName, requestCommand);

    if (strcmp(requestCommand, "run1") == 0) {
        sscanf(input, "%*s %*s %s", application);
        request_len = send_request(clientSocket, requestCommand, application, NULL);           // run app
    } else if (strcmp(requestCommand, "run2") == 0) {
        sscanf(input, "%*s %*s %s %[^\n]", application, arguments);
        request_len = send_request(clientSocket, requestCommand, application, arguments);      // run app argv
    } else if (strcmp(requestCommand, "kill1") == 0) {
        request_len = send_request(clientSocket, requestCommand, NULL, NULL);                  // kill (all children)
    } else if (strcmp(requestCommand, "kill2") == 0) {
        sscanf(input, "%*s %*s %s", application);
        request_len = send_request(clientSocket, requestCommand, application, NULL);           // kill pid
    } else {
        printf("Invalid command \"%s\"\n", requestCommand);
        return 111;
    }

    printf("serverSocket = %d\n", serverSocket);
    printf("clientSocket = %d\n", clientSocket);
    printf("request_len = %d\n", request_len);
    close_socket(clientSocket);
    close_host_socket(serverSocket);

    return 0;
}
