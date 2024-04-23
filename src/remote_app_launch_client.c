#include <stdio.h>  // printf, putchar, sscanf
#include <string.h> // strcmp, strlen
#include <stddef.h> // size_t
#include <stdlib.h> // strtol, malloc
#include <limits.h> // INT_MAX

#include "network_manager/network_manager.h"
#include "app_launcher/app_launcher.h"

struct Node {
    int pid;
    struct Node* next;
};

struct Node* add_process(struct Node** headPtr, int pid) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (newNode != NULL) {
        newNode->pid = pid;
        newNode->next = *headPtr;
        *headPtr = newNode;
    }
    return newNode;
}

void delete_process(struct Node** headPtr, int pid) {
    struct Node* current = *headPtr;
    struct Node* prev = NULL;
    while (current != NULL && current->pid != pid) {
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
    free(current);
}
void delete_all_processes(struct Node** headPtr) {
    struct Node* current = *headPtr;
    struct Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *headPtr = NULL;
}

long send_response(int clientSocket, const char* buffer) {
    size_t bufferLength = strlen(buffer);
    long requestLength = send_message(clientSocket, buffer, bufferLength + 1);
    if (requestLength == -1 || requestLength < bufferLength) {
        printf("Data not sent (%ld of %zu).\n", requestLength, bufferLength);
        return -1;
    }
    return requestLength;
}

void check_all_processes(const char* clientName, struct Node** pidsPtr, char* buffer) {
    struct Node* current = *pidsPtr;
    struct Node* next;
    int length;

    if (current == NULL) {
        sprintf(buffer, "Client \"%s\":  No running processes.", clientName);
        return;
    }
    length = sprintf(buffer, "Client \"%s\": ", clientName);
    while (current != NULL) {
        next = current->next;
        switch (get_process_state(current->pid)) {
            case 0:
                length += sprintf(buffer + length, "\n  Process %d is still running.", current->pid);
                break;
            case 1:
                length += sprintf(buffer + length, "\n  Process %d is exited.", current->pid);
                delete_process(pidsPtr, current->pid);
                break;
            case 2:
                length += sprintf(buffer + length, "\n  Process %d is terminated.", current->pid);
                delete_process(pidsPtr, current->pid);
                break;
            default: // -1
                length += sprintf(buffer + length, "\n  Get_process_status for %d is failed.", current->pid);
                delete_process(pidsPtr, current->pid);
                break;
        }
        current = next;
    }
}

// return pid created or killed, 1 for disconnecting, 0 for no commands received, -1 for error
long handle_request(int serverSocket, struct Node** pidsPtr) {
    char buffer[250]; // todo make static or global
    char requestCommand[20];
    char application[230];

    long requestLength = recv_message(serverSocket, buffer, sizeof(buffer));
//    printf("! handle requestLength = %zu\n", requestLength);
    if (requestLength <= 0) {
        return requestLength;
    }
    sscanf(buffer, "%s %[^\n]", requestCommand, application);
//    printf("! Receiving: \"%s\", \"%s\"\n", requestCommand, application);

    if (strcmp(requestCommand, "run") == 0) {
        int pid = launch_process(application);
        if (pid == -1) {
            printf("launch_process failed.\n");
            return -1;
        }
        add_process(pidsPtr, pid);
        return pid;
    } else if (strcmp(requestCommand, "kill") == 0) {
        if (strcmp (application, "all") == 0) { // disconnect client
            return 1;
        }
        long lpid = strtol(application, NULL, 10);
        if (lpid <= 0 || lpid >= INT_MAX) {
            return -1;
        }
        int pid = (int)lpid;
        // todo check for pid in pids
        if (terminate_process(pid) == -1) {
            printf("terminate_process \"%d\" failed.\n", pid);
            return -1;
        }
        delete_process(pidsPtr, pid);
        return pid;
    } else if (strcmp(requestCommand, "ok") == 0) {
//        printf("! ok\n");
        return 0;
    }
    printf("Unknown request received: \"%s\"\n", requestCommand);
    return -1;
}

int main(int argc, char* argv[]) {
    char output[300];
    char clientName[30];
    size_t clientNameLength = 0;
    long requestLength, quitSignal = 0;
    int clientSocket;
    struct Node* pids = NULL;

    if (argc != 2 || strlen(argv[1]) >= 30 || sscanf(argv[1], "%s", clientName) == EOF) {
        printf("Usage: %s <client name (length < 30)>\n", argv[0]);
        return 1;
    }
    printf("Client name is: \"%s\"\n", clientName);

    clientSocket = create_socket();
    if (clientSocket == -1) {
        printf("Failed to create socket.\n");
        return 1;
    }
    if (connect_to_server(clientSocket, "127.0.0.1", 8881) == -1) {
        printf("Connection failed.\n");
        return 4;
    }

    clientNameLength = strlen(clientName);
    requestLength = send_message(clientSocket, clientName, clientNameLength + 1);
    if (requestLength == -1 || requestLength < clientNameLength) {
        printf("Client name not send (%ld of %zu).\n", requestLength, clientNameLength);
        return 1;
    }

    int temp = 0;
    while (quitSignal >= 0) {
        printf("\n! temp %d\n", ++temp);
        if (quitSignal == 1) {
            sprintf(output, "Client \"%s\": disconnected.", clientName);
        } else {
            check_all_processes(clientName, &pids, output);
        }
        send_response(clientSocket, output);
        quitSignal = handle_request(clientSocket, &pids);
        sleep_wrapper(1);
    }
    close_host_socket(clientSocket);

//    printf("! Clean up\n"); // todo Ctrl-C
    struct Node* current = pids;
    while (current != NULL) {
        if (terminate_process(current->pid) == -1) {
            printf("terminate %d failed.\n", current->pid);
        }
        current = current->next;
    }

    delete_all_processes(&pids);
//    printf("! Clean up completed\n");
    return 0;
}
