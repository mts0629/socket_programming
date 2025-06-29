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

    while (true) {
        // Input a string
        printf("Sending > ");
        fgets(send_buf, BUF_SIZE, stdin);

        // Remove '\n' from the string
        char *newline = strchr(send_buf, '\n');
        *newline = '\0';

        // Send the string to the server
        int sent_size = sendto(sock_fd, send_buf, strlen(send_buf) + 1, 0,
                               (struct sockaddr *)&addr, sizeof(addr));
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // Receive an echo-backed data from the server
        struct sockaddr_storage server_addr;
        socklen_t addr_len = sizeof(server_addr);
        int recv_size = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *)&server_addr, &addr_len);
        if (recv_size == -1) {
            PRINT_ERROR("recv() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // If received size is 0, the connection is closed
        if (recv_size == 0) {
            break;
        }

        printf("Received: %s\n", recv_buf);
    }

    // Close the connection
    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
