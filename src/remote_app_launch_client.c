#include <stdio.h>

#include "network_manager/network_manager.h"
#include "app_launcher/app_launcher.h"

int main() {
    printf("Hello, Client!\n");
    char buffer[100] = {0};

    int clientSocket = create_socket();
    if (clientSocket == -1) {
        printf("Failed to create socket.\n");
        return 1;
    }
    if (connect_to_server(clientSocket, "127.0.0.1", 8881) == -1) {
        printf("Connection failed.\n");
        return 4;
    }


    int pids[2] = {0}; // do linked list

    // loop:

    int response_len = recv_message(clientSocket, buffer, sizeof(buffer));
    printf("response_len = %d\n", response_len);
    printf("Message received: %s\n", buffer);
//    for (int i = 0; i < sizeof(buffer) / sizeof(buffer[0]); i++) {
//        printf("%c", buffer[i]);
//    }
//    printf("\n");

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
