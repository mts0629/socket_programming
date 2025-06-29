#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common_defs.h"

// Buffer for sending/received data
static char send_buf[BUF_SIZE];
static char recv_buf[BUF_SIZE];

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s SERVER_ADDRESS PORT_NUMBER\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Server info
    char *srv_addr = argv[1];
    uint16_t srv_port = strtoul(argv[2], NULL, 10);

    // Create a socket with UDP (SOCK_DGRAM)
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) {
        PRINT_ERROR("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    // Create an address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(srv_port);
    addr.sin_addr.s_addr = inet_addr(srv_addr);

    // Bind the address to the socket
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
        -1) {
        PRINT_ERROR("bind() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    PRINT_FMT_INFO("Open a socket %s:%d\n", srv_addr, srv_port);

    while (true) {
        // Receive data from the client
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int recv_size = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *)&client_addr, &addr_len);
        if (recv_size == -1) {
            PRINT_ERROR("recv() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // If received size is 0, the connection is closed
        if (recv_size == 0) {
            break;
        }

        // Print the received data
        printf("Received: %s\n", recv_buf);

        // Echo back to the client
        memcpy(send_buf, recv_buf, recv_size);
        int sent_size =
            sendto(sock_fd, send_buf, strlen(send_buf) + 1, 0,
                   (struct sockaddr *)&client_addr, sizeof(client_addr));
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Close the connection
    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
