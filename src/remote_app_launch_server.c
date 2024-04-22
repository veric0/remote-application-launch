#include <stdio.h>  // printf, fgets, scanf, sprintf
#include <string.h> // strcmp, strlen
#include <stdlib.h> // malloc

#include "network_manager/network_manager.h"

struct command_node {
    char* requestCommand;
    char* application;
    struct command_node* next;
};

struct client_node {
    int clientSocket;
    char* clientName;
    struct command_node* commandsHead;
    struct client_node* next;
};

struct client_node* add_client(struct client_node** headPtr, int clientSocket, const char* clientName) {
    struct client_node* newNode = (struct client_node*)malloc(sizeof(struct client_node));
    if (newNode != NULL) {
        size_t len = strlen(clientName);
        newNode->clientName = (char *)malloc((len + 1) * sizeof(char));
        if (newNode->clientName == NULL) {
            printf("Failed to allocate memory for clientName\n");
            free(newNode);
            return NULL;
        }
        strcpy(newNode->clientName, clientName);
        newNode->clientSocket = clientSocket;
        newNode->commandsHead = NULL;
        newNode->next = *headPtr;
        *headPtr = newNode;
    }
    return newNode;
}
// todo add/remove command

struct client_node* find(struct client_node* node, int clientSocket) {
    while (node != NULL) {
        if (node->clientSocket == clientSocket) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

void delete(struct client_node** headPtr, int clientSocket) {
    struct client_node* current = *headPtr;
    struct client_node* prev = NULL;
    while (current != NULL && current->clientSocket != clientSocket) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) {
        return;
    }
    if (prev == NULL) {
        *headPtr = current->next;
    } else {
        prev->next = current->next;
    }
    free(current->clientName);
    free(current);
}

void deleteAll(struct client_node** headPtr) {
    struct client_node* current = *headPtr;
    struct client_node* next;
    while (current != NULL) {
        next = current->next;
        free(current->clientName);
        free(current);
        current = next;
    }
    *headPtr = NULL;
}

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
    struct client_node* clients = NULL;

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

    // loop:
    int clientSocket = accept_client(serverSocket);
    printf("! clientSocket = %d\n", clientSocket);
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

    clients = add_client(&clients, clientSocket, clientName);

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
            printf("! requestLength = %ld\n", requestLength);
        } else {
            printf("requestLength = %ld\n", requestLength);
            return -1;
        }
    }

    delete(&clients, clientSocket);
    close_socket(clientSocket);
    // end loop

    close_host_socket(serverSocket);

    if (clients != NULL) {
        deleteAll(&clients);
    }
    return 0;
}
