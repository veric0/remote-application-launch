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
    struct command_node* commandsQueueHead;
    struct command_node* commandsQueueTail;
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
        newNode->commandsQueueHead = NULL;
        newNode->commandsQueueTail = NULL;
        newNode->next = *headPtr;
        *headPtr = newNode;
    }
    return newNode;
}

struct client_node* find(struct client_node* node, int clientSocket) {
    while (node != NULL) {
        if (node->clientSocket == clientSocket) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

struct client_node* push_command(struct client_node* head, const char* clientName, const char* requestCommand, const char* application) {
    while (head != NULL) {
        if (strcmp(head->clientName, clientName) == 0) {
            break;
        }
        head = head->next;
    }
    if (head == NULL) {
        return NULL;
    }

    struct command_node* newNode = (struct command_node*)malloc(sizeof(struct command_node));
    size_t len1 = strlen(requestCommand);
    size_t len2 = strlen(application);
    newNode->requestCommand = (char *)malloc((len1 + len2 + 2) * sizeof(char));
    if (newNode->requestCommand == NULL) {
        printf("Failed to allocate memory for requestCommand and application\n");
        free(newNode);
        return NULL;
    }
    strcpy(newNode->requestCommand, requestCommand);
    newNode->application = newNode->requestCommand + len1 + 1;
    strcpy(newNode->application, application);
    newNode->next = NULL;

    if (head->commandsQueueHead == NULL) {
        head->commandsQueueHead = newNode;
        head->commandsQueueTail = newNode;
    } else {
        head->commandsQueueTail->next = newNode;
        head->commandsQueueTail = newNode;
    }
    return head;
}

struct client_node* pop_command(struct client_node* node) {
    if (node == NULL || node->commandsQueueHead == NULL) {
        return NULL;
    }

    if (node->commandsQueueHead->next == NULL) {
        node->commandsQueueTail = NULL;
    }
    struct command_node* temp = node->commandsQueueHead;
    node->commandsQueueHead = node->commandsQueueHead->next;

    free(temp->requestCommand); // requestCommand and application together
    free(temp);
    return node;
}

void delete_client(struct client_node** headPtr, int clientSocket) {
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

void delete_all_clients(struct client_node** headPtr) {
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

int is_run_command(const char* command) {
    return strcmp(command, "run") == 0;
}

int is_kill_command(const char* command, const char* application) {
    char *endPtr = application;
    // a && (b || (c && d))
    // Short-circuit evaluation: if (a) is true, (b) is evaluated. if (b) is true, (c && d) are not evaluated.
    return strcmp(command, "kill") == 0 &&
                (strcmp(application, "all") == 0 ||
                (strtol(application, &endPtr, 0) > 0 &&
                    endPtr != application));
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
    sscanf(input, "%s %s %[^\n]", clientName, requestCommand, application);

    if (is_run_command(requestCommand) ||
            is_kill_command(requestCommand, application)) {
        push_command(clients, clientName, requestCommand, application);
    } else {
        printf("Invalid command \"%s\" \"%s\"\n", requestCommand, application);
        return 11;
    }

    if (strcmp(clients->clientName, clientName) == 0) {
        requestLength = send_request(clients->clientSocket,
                                     clients->commandsQueueHead->requestCommand,
                                     clients->commandsQueueHead->application);
        printf("! requestLength = %ld\n", requestLength);
        pop_command(clients);

        for (int i = 0; i < 100; ++i) {
            printf("%d\n", i);
            requestLength = recv_message(clientSocket, input, sizeof(input));
            if (requestLength > 0) {
                puts(input);
            } else {
                printf("requestLength = %ld\n", requestLength);
                return -1;
            }
        }
        delete_client(&clients, clients->clientSocket);
    }

    close_socket(clientSocket);
    // end loop

    close_host_socket(serverSocket);

    if (clients != NULL) {
        delete_all_clients(&clients);
    }
    return 0;
}
