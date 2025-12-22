#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
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

bool running = true;

// Get an HTTP request method from a received data
static char *get_request_method(const char *recv_buf) {
    static char req_buf[8];

    int i = 0;
    for (; i < 7; i++) {
        if (recv_buf[i] == ' ') {
            break;
        }
        req_buf[i] = recv_buf[i];
    }

    req_buf[i] = '\0';

    return req_buf;
}

static bool str_eq(const char *s1, const char *s2) {
    return (strcmp(s1, s2) == 0) ? true : false;
}

char page_content[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta charset=\"utf-8\" />"
    "<title>hello</title>"
    "</head>"
    "<body>"
    "<h1>Hello</h1>"
    "<p>Hello, world!</p>"
    "</body>"
    "</html>";

// Create an HTTP response
static void create_page_response(char *buf, const size_t buf_size) {
    memset(buf, '\0', buf_size);

    snprintf(buf, buf_size,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "\r\n"
             "%s",
             strlen(page_content), page_content);
}

// Hander for SIGINT
static void handle_sigint(int sig) {
    (void)sig;
    running = false;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s SERVER_ADDRESS PORT_NUMBER\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Register handler (old way)
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        PRINT_ERROR("signal() failed\n");
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
    PRINT_INFO("Ready\n");

    while (running) {
        int conn_fd = accept(sock_fd, NULL, NULL);
        if (conn_fd == -1) {
            // Break when accept() is interrupted by a signal
            if (errno == EINTR) {
                break;
            }
            PRINT_ERROR("accept() failed\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

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

        char *req_method = get_request_method(recv_buf);

        if (str_eq(req_method, "GET")) {
            // If GET is requested, return a page content
            create_page_response(send_buf, sizeof(send_buf));
        } else {
            // Otherwise, return 405
            snprintf(send_buf, sizeof(send_buf),
                     "HTTP/1.1 405 Method Not Allowed\r\n");
        }

        int sent_size = send(conn_fd, send_buf, strlen(send_buf) + 1, 0);
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(conn_fd);
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        close(conn_fd);
    }

    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
