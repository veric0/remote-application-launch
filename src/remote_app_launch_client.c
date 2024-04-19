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
    int response_len = recv_message(clientSocket, buffer, sizeof(buffer));
    printf("response_len = %d\n", response_len);
    printf("Message received: %s\n", buffer);
//    for (int i = 0; i < sizeof(buffer) / sizeof(buffer[0]); i++) {
//        printf("%c", buffer[i]);
//    }
//    printf("\n");
    close_host_socket(clientSocket);

    return 0;
}
