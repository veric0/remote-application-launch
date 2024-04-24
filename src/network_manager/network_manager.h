/**
 * Wrapper interface for both Windows winsock2.h and Linux sys/socket.h.
 */
#ifndef REMOTE_APPLICATION_LAUNCH_NETWORK_MANAGER_H
#define REMOTE_APPLICATION_LAUNCH_NETWORK_MANAGER_H

#include <stdint.h>

/**
 * Create and return socket
 * @return server socket
 */
int create_socket();

/**
 * Bind port to socket
 * @return 0 on success, -1 on error
 */
int bind_socket(int serverSocket, uint16_t port);

/**
 * Listen to connections to socket
 * @return 0 on success, -1 on error
 */
int listen_port(int serverSocket, int n);

/**
 * Initiate connection to a server socket by given IP and port
 * @param clientSocket client socket
 * @param ipAddr server IP address
 * @param port server port
 * @return 0 on success, -1 on error
 */
int connect_to_server(int clientSocket, const char* ipAddr, uint16_t port);

/**
 * Wait for ready connections to socket. Maximum time to wait is 1 second.
 * @return number of ready connections, -1 on error, 0 on timeout
 */
int select_clients(int serverSocket);

/**
 * Accept a connection on the socket
 * @return client socket on success, -1 on error
 */
int accept_client(int serverSocket);

/**
 * Seng a message on a socket
 * @param clientSocket destination socket
 * @param message message to send
 * @param length length of message - bytes to send
 * @return number sent or -1 on error
 */
long send_message(int clientSocket, const char* message, size_t length);

/**
 * Receive a message from a socket
 * @param clientSocket source socket
 * @param buffer place to store message
 * @param length number of bytes to read
 * @return number received or -1 on error
 */
long recv_message(int clientSocket, char* buffer, size_t length);

/**
 * Close client socket
 * @return 0 on success, -1 on error
 */
int close_socket(int clientSocket);

/**
 * Close host socket. Same as close_socket(), but on Windows also calls WSACleanup()
 * @return 0 on success, -1 on error
 */
int close_host_socket(int hostSocket);

#endif //REMOTE_APPLICATION_LAUNCH_NETWORK_MANAGER_H
