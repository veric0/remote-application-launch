#include <stdio.h>  // printf, fgets, scanf, sprintf
#include <string.h> // strcmp, strlen
#include <stdlib.h> // malloc
#include <pthread.h>

#include "network_manager/network_manager.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int quitSignal = 0, serverSocket = -1;
const int MAX_CLIENTS = 1;
struct client_node* clients = NULL;

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

void* input_thread_func(void* vargp) {
    char input[300];
    char clientName[30];
    char requestCommand[30];
    char application[250];

//    while (1) {
        printf("Enter client name and command (run/kill) or (q) for exit:\n> ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%s %s %[^\n]", clientName, requestCommand, application);
        if (is_run_command(requestCommand) || is_kill_command(requestCommand, application)) {
            pthread_mutex_lock(&mutex);
            push_command(clients, clientName, requestCommand, application);
            pthread_mutex_unlock(&mutex);
        } else if (strcmp(clientName, "q") == 0) { // if first word is (q) // todo fix reading 2 words
            quitSignal = 1;
//            pthread_exit(NULL);
        } else {  // todo fiw deadlock recv
            printf("Invalid command \"%s\", \"%s\". Try again.\n", requestCommand, application);
        }
//    }
    return NULL;
}

// todo synchronize send and recv for multiple clients ?
void* handle_client(void* vargp) {
    char buffer[300];
    char clientName[30];
    long requestLength = 0;
    int clientSocket = *((int*)vargp);
    struct client_node* client;

    requestLength = recv_message(clientSocket, clientName, sizeof(clientName));
    if (requestLength < 0) {
        printf("Client name: \"%s\"\nrequestLength = %ld\n", clientName, requestLength);
        close_socket(clientSocket);
        pthread_exit(NULL);
    }
    printf("Client name: \"%s\"\n! requestLength = %ld\n", clientName, requestLength);

    pthread_mutex_lock(&mutex);
    client = add_client(&clients, clientSocket, clientName);
    pthread_mutex_unlock(&mutex);

    while (!quitSignal) {
        input_thread_func(NULL); // todo move
        pthread_mutex_lock(&mutex);
        if (client->commandsQueueHead != NULL) {
            send_request(client->clientSocket,
                         client->commandsQueueHead->requestCommand,
                         client->commandsQueueHead->application);
            pop_command(client);
        }
        pthread_mutex_unlock(&mutex);

        requestLength = recv_message(clientSocket, buffer, sizeof(buffer));
        if (requestLength > 0) {
            puts(buffer);
            char *ptr = strstr(buffer, "\": disconnected.");
            if (ptr) {
                break;
            }
        } else {
            printf("requestLength = %ld\n", requestLength);
            return NULL;
        }

    }
    pthread_mutex_lock(&mutex);
    delete_client(&client, client->clientSocket);
    pthread_mutex_unlock(&mutex);

    close_socket(clientSocket);
    return NULL;
}

int main() {
//    pthread_t inputThreadId;
//    pthread_t clientThreadsIds[MAX_CLIENTS];

    serverSocket = create_socket();
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

//    pthread_create(&inputThreadId, NULL, input_thread_func, NULL);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int clientSocket = accept_client(serverSocket);
        if (clientSocket == -1) {
            printf("Accept failed.\n");
            continue;
        }
        handle_client((void*)(&clientSocket));
//        pthread_create(clientThreadsIds + i, NULL, handle_client, NULL);
    }

//    pthread_join(inputThreadId, NULL);
//    for (int i = 0; i < MAX_CLIENTS; ++i) {
//        pthread_join(clientThreadsIds[i], NULL);
//    }

    close_host_socket(serverSocket);

    if (clients != NULL) {
        delete_all_clients(&clients);
    }
    return 0;
}
