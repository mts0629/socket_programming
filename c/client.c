#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common_defs.h"

// Buffer for sending data
static char send_buf[BUF_SIZE];

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

    // Connect to a server
    PRINT_INFO("Connect to a server...\n");
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        PRINT_ERROR("connect() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    PRINT_INFO("Connected\n");

    while (true) {
        // Input a string
        printf("Send > ");
        fgets(send_buf, BUF_SIZE, stdin);

        // Remove '\n' from the string
        char *newline = strchr(send_buf, '\n');
        *newline = '\0';

        // Send the string to the server
        int sent_size = send(sock_fd, send_buf, strlen(send_buf) + 1, 0);
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Close connection
    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
