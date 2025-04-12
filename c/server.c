#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common_defs.h"

// Buffer for received data
static char recv_buf[BUF_SIZE];

int main(void) {
    // Create a socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        PRINT_ERROR("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    // Create an address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SRV_PORT);
    addr.sin_addr.s_addr = inet_addr(SRV_ADDR);

    // Bind the address to the socket
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        PRINT_ERROR("bind() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Wait connection
    if (listen(sock_fd, 1) == -1) {
        PRINT_ERROR("listen() falied\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Wait connection
    PRINT_INFO("Wait connection from a client...\n");
    int conn_fd = accept(sock_fd, NULL, NULL);
    if (conn_fd == -1) {
        PRINT_ERROR("accept() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    PRINT_INFO("Connected\n");

    while (true) {
        // Receive data from the client
        int recv_size = recv(conn_fd, recv_buf, BUF_SIZE, 0);
        if (recv_size == -1) {
            PRINT_ERROR("recv() failed\n");
            close(conn_fd);
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // If receive size is 0, connection is closed
        if (recv_size == 0) {
            break;
        }

        // Print the received data
        printf("Received > %s\n", recv_buf);
    }

    // Close connection
    close(conn_fd);
    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
