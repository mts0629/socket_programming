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

// Buffer for received data
#define RECV_BUF_SIZE 1024
static char recv_buf[RECV_BUF_SIZE];

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

    // Bind the address to the socket
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        print_error("bind() failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Wait connection
    if (listen(sock_fd, 1) == -1) {
        print_error("listen() falied");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Wait connection
    print_info("Wait connection from a client...");
    int conn_fd = accept(sock_fd, NULL, NULL);
    if (conn_fd == -1) {
        print_error("accept() failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    print_info("Connected");

    while (true) {
        // Receive data from the client
        int recv_size = recv(conn_fd, recv_buf, RECV_BUF_SIZE, 0);
        if (recv_size == -1) {
            print_error("recv() failed");
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
    print_info("Connection closed");

    return EXIT_SUCCESS;
}
