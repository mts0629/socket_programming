#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Buffer info
#define BUF_SIZE 1024

// Print infomation message
#define PRINT_INFO(str) printf("[Info] " str)
#define PRINT_FMT_INFO(fmt_str, ...) printf("[Info] " fmt_str, __VA_ARGS__)

// Print error message
#define PRINT_ERROR(str) printf("[Error] " str)

// Buffer for sending/received data
static char send_buf[BUF_SIZE];
static char recv_buf[BUF_SIZE];

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s SERVER_ADDRESS PORT_NUMBER\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *srv_addr = argv[1];
    uint16_t srv_port = strtoul(argv[2], NULL, 10);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        PRINT_ERROR("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(srv_port);
    addr.sin_addr.s_addr = inet_addr(srv_addr);

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
        -1) {
        PRINT_ERROR("bind() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    PRINT_FMT_INFO("Open a socket %s:%d\n", srv_addr, srv_port);

    if (listen(sock_fd, 1) == -1) {
        PRINT_ERROR("listen() falied\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    PRINT_INFO("Wait connection from a client...\n");
    int conn_fd = accept(sock_fd, NULL, NULL);
    if (conn_fd == -1) {
        PRINT_ERROR("accept() failed\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    PRINT_INFO("Connected\n");

    while (true) {
        int recv_size = recv(conn_fd, recv_buf, BUF_SIZE, 0);
        if (recv_size == -1) {
            PRINT_ERROR("recv() failed\n");
            close(conn_fd);
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        if (recv_size == 0) {
            break;
        }

        snprintf(send_buf, sizeof(send_buf), "HTTP/1.1 200 OK\n");

        int sent_size = send(conn_fd, send_buf, strlen(send_buf) + 1, 0);
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(conn_fd);
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
    }

    close(conn_fd);
    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
