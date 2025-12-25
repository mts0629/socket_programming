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
// Content of HTTP response
static char content[BUF_SIZE];
// Path to resources
static char resource_root[BUF_SIZE];

bool running = true;

// Get an HTTP request method from a received data
void get_request_method(char *req_buf, const size_t buf_size,
                        const char *recv_buf) {
    size_t i = 0;
    for (; i < buf_size; i++) {
        if (recv_buf[i] == ' ') {
            break;
        }
        req_buf[i] = recv_buf[i];
    }

    req_buf[i] = '\0';
}

static bool str_eq(const char *s1, const char *s2) {
    return (strcmp(s1, s2) == 0) ? true : false;
}

// Get URI from an HTTP request
bool get_uri(char *uri, const size_t buf_size, const char *recv_buf) {
    size_t i = 0;
    while (recv_buf[i] != ' ') {
        if ((recv_buf[i] == '\0') || (i == (buf_size - 1))) {
            return false;
        }

        i++;
    }

    i++;

    size_t j = 0;
    while (recv_buf[i] != ' ') {
        if ((recv_buf[i] == '\0') || (i == (buf_size - 1))) {
            return false;
        }

        uri[j] = recv_buf[i];
        i++;
        j++;
    }

    i++;

    uri[i] = '\0';

    return true;
}

// Find a resource from URI
bool find_resource(const char *uri, char *path_buf, const size_t buf_size) {
    strncpy(path_buf, resource_root, buf_size);
    size_t len = strlen(path_buf);

    if (str_eq(uri, "/")) {
        strncpy((path_buf + len), "/index.html", 12);
    } else {
        strncpy((path_buf + len), uri, strlen(uri));
    }

    FILE *fp = fopen(path_buf, "r");
    if (fp == NULL) {
        return false;
    }

    fclose(fp);

    return true;
}

// Create an HTTP response
static void create_response(char *buf, const size_t buf_size,
                            const char *path) {
    memset(buf, '\0', buf_size);

    FILE *fp = fopen(path, "r");

    int i = 0;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        content[i] = (char)c;
        i++;
    }

    fclose(fp);

    snprintf(buf, buf_size,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "\r\n"
             "%s",
             strlen(content), content);
}

// Hander for SIGINT
static void handle_sigint(int sig) {
    (void)sig;
    running = false;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s SERVER_ADDRESS PORT_NUMBER [RESOURCE_ROOT]\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    // Register handler (old way)
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        PRINT_ERROR("signal() failed\n");
        exit(EXIT_FAILURE);
    }

    // Specify the root of resources
    if (argc > 3) {
        strncpy(resource_root, argv[3], sizeof(resource_root));
    } else {
        strncpy(resource_root, ".", sizeof(resource_root));
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

        char req_method[8];
        get_request_method(req_method, sizeof(req_method), recv_buf);

        if (str_eq(req_method, "GET")) {
            char uri[BUF_SIZE];
            if (get_uri(uri, sizeof(uri), recv_buf)) {
                char path[BUF_SIZE];
                if (find_resource(uri, path, sizeof(path))) {
                    create_response(send_buf, sizeof(send_buf), path);
                } else {
                    snprintf(send_buf, sizeof(send_buf),
                             "HTTP/1.1 404 Not Found\r\n");
                }
            } else {
                snprintf(send_buf, sizeof(send_buf),
                         "HTTP/1.1 400 Bad Request\r\n");
            }
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
