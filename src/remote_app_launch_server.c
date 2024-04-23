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

struct client_node* push_command(struct client_node* head, const char* clientName, const char* requestCommand, const char* application) {
//    printf("!!! push client1 %p, c = \"%s\", rc = \"%s\", a = \"%s\"\n", head, clientName, requestCommand, application);
    while (head != NULL) {
//        printf("!!! push loop \"%s\", c = \"%s\"\n", head->clientName, clientName);
        if (strcmp(head->clientName, clientName) == 0) {
            break;
        }
        head = head->next;
    }
//    printf("!!! push client2 %p, c = \"%s\", rc = \"%s\", a = \"%s\"\n", head, clientName, requestCommand, application);
    if (head == NULL) {
        return NULL;
    }
//    printf("!!! push client3 \"%s\", c = \"%s\", rc = \"%s\", a = \"%s\"\n", head->clientName, clientName, requestCommand, application);
//    printf("!!! push node queue before \"%s\", h = %p, t = %p\n", head->clientName, head->commandsQueueHead, head->commandsQueueTail);

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
//    printf("!!! push node queue after \"%s\", h = %p, t = %p\n", head->clientName, head->commandsQueueHead, head->commandsQueueTail);
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

    while (1) {
        printf("Enter client name and command (run/kill) or (q) for exit:\n> ");
        fgets(input, sizeof(input), stdin);
//        printf("!! input \"%s\"\n", input);
        sscanf(input, "%s", clientName);
        if (strcmp(clientName, "q") == 0) { // if first word is (q)
            quitSignal = 1;
            pthread_exit(NULL);
        } else {
            sscanf(input, "%*s %s %[^\n]", requestCommand, application);
//            printf("!! input c = \"%s\", rc = \"%s\", a = \"%s\"\n", clientName, requestCommand, application);
            if (is_run_command(requestCommand) || is_kill_command(requestCommand, application)) {
//                printf("!! input push\n");
                pthread_mutex_lock(&mutex);
//                printf("!! input lock\n");
                push_command(clients, clientName, requestCommand, application);
//                printf("!! input lock2\n");
                pthread_mutex_unlock(&mutex);
//                printf("!! input unlock\n");
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
            printf("Sending: \"%s\", \"%s\"\n", requestCommand, application);
            sprintf(request, "%s %s", requestCommand, application);
        } else {
            printf("Sending: \"%s\"\n", requestCommand);
            sprintf(request, "%s", requestCommand);
        }
    } else {
        return -1;
    }
    requestLength = send_message(clientSocket, request, strlen(request) + 1);
//    printf("! send request %ld \"%s\"\n", requestLength, request);
    if (requestLength < 0) {
        return -1;
    }
    return requestLength;
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
    printf("Client name: \"%s\"\n! recv requestLength = %ld\n", clientName, requestLength);

    pthread_mutex_lock(&mutex);
    client = add_client(&clients, clientSocket, clientName);
//    printf("! client %p, %d, \"%s\"\n", client, client->clientSocket, client->clientName);
    pthread_mutex_unlock(&mutex);

    send_request(client->clientSocket, "ok", "ok");

    int temp = 0;
    while (!quitSignal) {
        printf("\n! temp %d\n", ++temp);
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
        pthread_mutex_lock(&mutex);
//        printf("! client queue before \"%s\", h = %p, t = %p\n", client->clientName, client->commandsQueueHead, client->commandsQueueTail);
        if (client->commandsQueueHead != NULL) {
//            printf("! before pop command %p, rc = \"%s\", a = \"%s\", n = %p\n", client->commandsQueueHead, client->commandsQueueHead->requestCommand, client->commandsQueueHead->application, client->commandsQueueHead->next);
            send_request(client->clientSocket,
                         client->commandsQueueHead->requestCommand,
                         client->commandsQueueHead->application);
            pop_command(client);
        } else {
            send_request(client->clientSocket, "ok", "ok");
        }
//        printf("! client queue after \"%s\", h = %p, t = %p\n", client->clientName, client->commandsQueueHead, client->commandsQueueTail);
        pthread_mutex_unlock(&mutex);

    }
    pthread_mutex_lock(&mutex);
    delete_client(&client, client->clientSocket);
    pthread_mutex_unlock(&mutex);

    close_socket(clientSocket);
    return NULL;
}

int main() {
    pthread_t inputThreadId;
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

    pthread_create(&inputThreadId, NULL, input_thread_func, NULL);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int clientSocket = accept_client(serverSocket);
        if (clientSocket == -1) {
            printf("Accept failed.\n");
            continue;
        }
        handle_client((void*)(&clientSocket));
//        pthread_create(clientThreadsIds + i, NULL, handle_client, NULL); // todo handle client in new thread
    }

//    printf("! cancel input\n");
    pthread_cancel(inputThreadId);
//    printf("! join input\n");
    pthread_join(inputThreadId, NULL);
//    printf("! join completed\n");

//    for (int i = 0; i < MAX_CLIENTS; ++i) {
//        pthread_join(clientThreadsIds[i], NULL);
//    }

    close_host_socket(serverSocket);

    if (clients != NULL) {
        delete_all_clients(&clients);
    }
    return 0;
}
