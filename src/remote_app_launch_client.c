#include <stdio.h>  // printf, putchar, sscanf
#include <string.h> // strcmp, strlen
#include <stddef.h> // size_t
#include <stdlib.h> // strtol
#include <limits.h> // INT_MAX

#include "network_manager/network_manager.h"
#include "app_launcher/app_launcher.h"


int receive_request(int serverSocket, int* pids) {
    char buffer[250];
    char requestCommand[20];
    char application[230];

    int requestLength = recv_message(serverSocket, buffer, sizeof(buffer));
    if (requestLength < 0) {
        return -1;
    }
    sscanf(buffer, "%s %[^\n]", requestCommand, application);
    printf("Receiving: \"%s\", \"%s\"", requestCommand, application);
    printf("\nreceived_len = %d\n", requestLength);

    if (strcmp(requestCommand, "run1") == 0 || strcmp(requestCommand, "run2") == 0) {
        pids[0] = launch_process(application);
        if (pids[0] == -1) {
            printf("launch_process failed.\n");
            return -1;
        }
        return pids[0];
    } else if (strcmp(requestCommand, "kill1") == 0) {
//        in loop terminate_process();
    } else if (strcmp(requestCommand, "kill2") == 0) {
        long lpid = strtol(application, NULL, 10);
        if (lpid <= 0 || lpid >= INT_MAX) {
            return -1;
        }
        return terminate_process((int)lpid);
    } else {
        printf("\nОтримано невідомий запит: \"%s\"", requestCommand);
    }
    return requestLength;
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
    printf("requestLength = %d\n", requestLength);

    int pids[2] = {0}; // do linked list

    // send name to server
    // loop:

    receive_request(clientSocket, pids);

//    char gedit[] = "/usr/bin/gedit 123.txt";
//    pids[0] = launch_process(gedit);
    if (pids[0] == -1) {
        printf("launch_process failed.\n");
        return 6; // continue loop
    }
    for (int i = 0; i < 30; ++i) {
        for (int p = 0; p < 2; ++p) {
            if (pids[p] == 0) break;
            switch (get_process_state(pids[p])) {
                case 0:
                    printf("process %d is still running.\n", pids[p]);
                    break;
                case 1:
                    printf("process %d is exited.\n", pids[p]);
                    pids[p] = 0;
                    break;
                case 2:
                    printf("process %d is terminated.\n", pids[p]);
                    pids[p] = 0;
                    break;
                default: // -1
                    printf("get_process_status for %d is failed.\n", pids[p]);
                    pids[p] = 0;
                    break; // return
            }
        }
        putchar('\n');
        sleep_wrapper(1);
    }

    printf("terminate:\n");
    if (terminate_process(pids[0]) == -1) {
        printf("terminate failed.\n");
        return 8;
    } // else send response success terminate

    // end loop

    close_host_socket(clientSocket);

    return 0;
}
