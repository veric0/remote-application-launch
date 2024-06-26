#include <stdio.h>  // printf, puts, fgets, sscanf, sprintf
#include <string.h> // strcmp, strlen, strcpy
#include <stdlib.h> // malloc, free, strtol
#include <pthread.h>

#include "network_manager/network_manager.h"

// global variables used by all threads:
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int quitSignal = 0, serverSocket = -1, activeConnectionsCount = 0;
const int MAX_CLIENTS = 42;
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
            puts("Failed to allocate memory for client name.");
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
        puts("Failed to allocate memory for command and application.");
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

void delete_commands(struct client_node* client) {
    if (client == NULL) {
        return;
    }
    struct command_node* temp = NULL;
    while (client->commandsQueueHead != NULL) {
        temp = client->commandsQueueHead;
        client->commandsQueueHead = client->commandsQueueHead->next;
        free(temp->requestCommand); // requestCommand and application together
        free(temp);
    }
}

void delete_client(struct client_node** headPtr, int clientSocket) {
    struct client_node* current = *headPtr;
    struct client_node* prev = NULL;

    // find client node
    while (current != NULL && current->clientSocket != clientSocket) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) {
        return;
    }
    // change links
    if (prev == NULL) {
        *headPtr = current->next;
    } else {
        prev->next = current->next;
    }
    delete_commands(current);
    free(current->clientName);
    free(current);
}

void delete_all_clients(struct client_node** headPtr) {
    struct client_node* current = *headPtr;
    struct client_node* next;
    while (current != NULL) {
        next = current->next;
        delete_commands(current);
        free(current->clientName);
        free(current);
        current = next;
    }
    *headPtr = NULL;
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
                (strtol(application, &endPtr, 0) > 0l &&
                    endPtr != application));
}

void* input_thread_func(void* vargp) {
    char input[300];
    char clientName[30];
    char requestCommand[30];
    char application[250];

    while (1) {
        printf("Enter client name and command (run/kill) or (q) for exit:\n> ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%s", clientName);
        if (strcmp(clientName, "q") == 0) { // if first word is (q)
            quitSignal = 1;
            pthread_exit(NULL);
        } else {
            sscanf(input, "%*s %s %[^\n]", requestCommand, application);
            if (is_run_command(requestCommand) || is_kill_command(requestCommand, application)) {
                pthread_mutex_lock(&mutex);
                push_command(clients, clientName, requestCommand, application);
                pthread_mutex_unlock(&mutex);
            } else {
                printf("Invalid command \"%s\", \"%s\". Try again.\n", requestCommand, application);
            }
        }
    }
}

long send_request(int clientSocket, const char* requestCommand, const char* application) {
    char request[250];
    long requestLength;

    if (requestCommand != NULL) {
        if (application != NULL) {
            sprintf(request, "%s %s", requestCommand, application);
        } else {
            sprintf(request, "%s", requestCommand);
        }
    } else {
        return -1;
    }
    requestLength = send_message(clientSocket, request, strlen(request) + 1);
    if (requestLength < 0) {
        return -1;
    }
    return requestLength;
}

void* handle_client(void* vargp) {
    char buffer[300];
    char clientName[30];
    long requestLength = 0;
    struct client_node* client;
    int clientSocket = *((int*)vargp);

    requestLength = recv_message(clientSocket, clientName, sizeof(clientName));
    if (requestLength < 0) {
        close_socket(clientSocket);
        --activeConnectionsCount;
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&mutex);
    client = add_client(&clients, clientSocket, clientName);
    pthread_mutex_unlock(&mutex);

    send_request(client->clientSocket, "ok", "ok");
    printf("Client \"%s\" connected.\n", clientName);

    while (!quitSignal) {
        requestLength = recv_message(clientSocket, buffer, sizeof(buffer));
        if (requestLength > 0) {
            if (strcmp(buffer, "ok") != 0) {
                puts(buffer);
            }
        } else {
            printf("Unable to read request from client \"%s\".\n", clientName);
        }
        pthread_mutex_lock(&mutex);
        if (client->commandsQueueHead != NULL) {
            send_request(client->clientSocket,
                         client->commandsQueueHead->requestCommand,
                         client->commandsQueueHead->application);
            if (strcmp(client->commandsQueueHead->application, "all") == 0) {
                pop_command(client);
                pthread_mutex_unlock(&mutex);
                break; // disconnect client
            }
            pop_command(client);
        } else {
            send_request(client->clientSocket, "ok", "ok");
        }
        pthread_mutex_unlock(&mutex);

    }
    pthread_mutex_lock(&mutex);
    delete_client(&clients, client->clientSocket);
    pthread_mutex_unlock(&mutex);

    close_socket(clientSocket);
    printf("Client \"%s\" disconnected.\n", clientName);
    --activeConnectionsCount;
    pthread_exit(NULL);
}

// stack
struct thread_id_node {
    pthread_t threadId;
    struct thread_id_node* next;
};

struct thread_id_node* create_thread_id() {
    return (struct thread_id_node*)malloc(sizeof(struct thread_id_node));
}

int main() {
    struct thread_id_node* inputThreadId = create_thread_id();
    struct thread_id_node *threads, *node;
    inputThreadId->next = NULL;
    threads = inputThreadId;

    serverSocket = create_socket();
    if (serverSocket == -1) {
        puts("Failed to create socket.");
        return 1;
    }
    if (bind_socket(serverSocket, 8881) == -1) {
        puts("Bind failed.");
        return 2;
    }
    if (listen_port(serverSocket, MAX_CLIENTS) == -1) {
        puts("Listen failed.");
        return 3;
    }

    pthread_create(&(inputThreadId->threadId), NULL, input_thread_func, NULL);

    while (!quitSignal) {
        if (activeConnectionsCount < MAX_CLIENTS && select_clients(serverSocket) > 0) {
            int clientSocket = accept_client(serverSocket);
            if (clientSocket == -1) {
                puts("Accept failed.");
                continue;
            }
            node = create_thread_id();
            node->next = threads;
            threads = node;
            ++activeConnectionsCount;
            pthread_create(&(node->threadId), NULL, handle_client, (void *) (&clientSocket));
        }
    }

    pthread_cancel(inputThreadId->threadId);

    while (threads != NULL) {
        node = threads;
        threads = threads->next;
        pthread_join(node->threadId, NULL);
        free(node);
    }

    close_host_socket(serverSocket);

    if (clients != NULL) {
        puts("Disconnecting clients...");
        delete_all_clients(&clients);
    }
    puts("Server shutdown.");
    return 0;
}
