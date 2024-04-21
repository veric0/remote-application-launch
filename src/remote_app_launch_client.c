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

struct Node* append(struct Node** headPtr, int pid) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (newNode != NULL) {
        newNode->pid = pid;
        newNode->next = *headPtr;
        *headPtr = newNode;
    }
    return newNode;
}

struct Node* find(struct Node* head, int pid) {
    while (head != NULL) {
        if (head->pid == pid) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void delete(struct Node** headPtr, int pid) {
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
void deleteAll(struct Node** headPtr) {
    struct Node* current = *headPtr;
    struct Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *headPtr = NULL;
}

int handle_request(int serverSocket, struct Node** pidsPtr) {
    char buffer[250];
    char requestCommand[20];
    char application[230];

    int requestLength = recv_message(serverSocket, buffer, sizeof(buffer));
    if (requestLength < 0) {
        return -1;
    }
    sscanf(buffer, "%s %[^\n]", requestCommand, application);
    printf("! Receiving: \"%s\", \"%s\"", requestCommand, application);
    printf("\n! received_len = %d\n", requestLength);

    if (strcmp(requestCommand, "run1") == 0 || strcmp(requestCommand, "run2") == 0) {
        int pid = launch_process(application);
        if (pid == -1) {
            printf("launch_process failed.\n");
            return -1;
        } else {
            append(pidsPtr, pid);
            // response success to server
        }
        return pid;
    } else if (strcmp(requestCommand, "kill1") == 0) {
//        todo in loop terminate_process();
    } else if (strcmp(requestCommand, "kill2") == 0) {
        long lpid = strtol(application, NULL, 10);
        if (lpid <= 0 || lpid >= INT_MAX) {
            return -1;
        }
        int pid = (int)lpid;
        if (terminate_process(pid) == 0) {
            delete(pidsPtr, pid);
            return 0;
        } else {
            printf("terminate_process failed.\n");
            return -1;
        }
    } else {
        printf("Unknown request received: \"%s\"", requestCommand);
    }
    return requestLength;
}

// todo return string and send to server
void check_all_processes(const char* clientName, struct Node** pidsPtr) {
    struct Node* current = *pidsPtr;
    struct Node* next;
    if (current == NULL) {
        return;
    }
    while (current != NULL) {
        next = current->next;
        switch (get_process_state(current->pid)) {
            case 0:
                printf("Client \"%s\": process %d is still running.\n", clientName, current->pid);
                break;
            case 1:
                printf("Client \"%s\": process %d is exited.\n", clientName, current->pid);
                delete(pidsPtr, current->pid);
                break;
            case 2:
                printf("Client \"%s\": process %d is terminated.\n", clientName, current->pid);
                delete(pidsPtr, current->pid);
                break;
            default: // -1
                printf("Client \"%s\": get_process_status for %d is failed.\n", clientName, current->pid);
                delete(pidsPtr, current->pid);
                break;
        }
        current = next;
    }
}

int main(int argc, char* argv[]) {
    char clientName[30];
    size_t clientNameLength = 0;
    int clientSocket, requestLength;

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
    requestLength = send_message(clientSocket, clientName, clientNameLength);
    if (requestLength == -1 || requestLength < clientNameLength) {
        printf("Client name not send (%d of %zu).\n", requestLength, clientNameLength);
        return 1;
    }
    printf("! requestLength = %d\n", requestLength);

    struct Node* pids = NULL;

    handle_request(clientSocket, &pids);
//    char gedit[] = "/usr/bin/gedit 123.txt";
//    int pid = launch_process(gedit);
//    if (pid == -1) {
//        printf("launch_process failed.\n");
//        return 6;
//    } else {
//        append(&pids, pid);
//    }

    while (pids != NULL) {
        // todo handle_requests
        check_all_processes(clientName, &pids);
        // todo send response
        sleep_wrapper(1);
    }

    printf("! Clean up\n"); // todo Ctrl-C
    struct Node* current = pids;
    while (current != NULL) {
        if (terminate_process(current->pid) == -1) {
            printf("terminate %d failed.\n", current->pid);
        }
        current = current->next;
    }

    close_host_socket(clientSocket);
    deleteAll(&pids);
    printf("! Clean up completed\n");
    return 0;
}
