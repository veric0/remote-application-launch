#include <stdio.h> // printf, putchar, sscanf
#include <string.h> // strcmp

#include "network_manager/network_manager.h"
#include "app_launcher/app_launcher.h"


int receive_request(int serverSocket) {
    char buffer[250];
    char requestCommand[30];
    char application[100];
    char arguments[100];

    int received_len = recv_message(serverSocket, buffer, sizeof(buffer));
    if (received_len < 0) {
        return -1;
    }
    sscanf(buffer, "%s %s %[^\n]", requestCommand, application, arguments);
    printf("Receiving: \"%s\", \"%s\", \"%s\"", requestCommand, application, arguments);
    printf("\nreceived_len = %d\n", received_len);

    if (strcmp(requestCommand, "run1") == 0) {
//        launch_process();
    } else if (strcmp(requestCommand, "run2") == 0) {
//        launch_process(); with argv
    } else if (strcmp(requestCommand, "kill1") == 0) {
//        in loop terminate_process();
    } else if (strcmp(requestCommand, "kill2") == 0) {
//        terminate_process();
    } else {
        printf("\nОтримано невідомий запит: \"%s\"", requestCommand);
    }
    return received_len;
}

int main(int argc, char* argv[]) {
    char* clientName;
    int clientSocket;

    if (argc != 2) {
        printf("Usage: %s <client name>\n", argv[0]);
        return 1;
    }
    clientName = argv[1];
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

    int pids[2] = {0}; // do linked list

    // send name to server
    // loop:

    receive_request(clientSocket);

    pids[0] = launch_process("echo ! hello from child 1", NULL);
    pids[1] = launch_process("echo ! hello from child 2", NULL);
    if (pids[0] == -1) {
        printf("launch_process failed.\n");
        return 6; // continue loop
    }
    for (int i = 0; i < 3; ++i) {
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

    for (int i = 0; i < 5; ++i) {
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

    // end loop

    close_host_socket(clientSocket);

    return 0;
}
