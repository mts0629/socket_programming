#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Server info
#define SRV_ADDR "127.0.0.1"
#define SRV_PORT 8080

// Buffer for sending data
#define SEND_BUF_SIZE 1024
static char send_buf[SEND_BUF_SIZE];

static void print_info(const char *str) {
    printf("[Info] %s\n", str);
}

static void print_error(const char *str) {
    printf("[Error] %s\n", str);
}

int main(void) {
    // Create a socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        print_error("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Create an address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SRV_PORT);
    addr.sin_addr.s_addr = inet_addr(SRV_ADDR);

    // Connect to a server
    print_info("Connect to a server...");
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        print_error("connect() failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    print_info("Connected");

    while (true) {
        // Input a string
        printf("Send > ");
        fgets(send_buf, sizeof(send_buf), stdin);

        // Remove '\n' from the string
        char *newline = strchr(send_buf, '\n');
        *newline = '\0';

        // Send the string to the server
        int sent_size = send(sock_fd, send_buf, strlen(send_buf) + 1, 0);
        if (sent_size == -1) {
            print_error("send() failed");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Close connection
    close(sock_fd);
    print_info("Connection closed");

    return EXIT_SUCCESS;
}
