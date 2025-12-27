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

typedef enum { REQUEST_GET, REQUEST_NONE } RequestMethod;

typedef struct {
    RequestMethod method;
    char uri[BUF_SIZE];
    char version[16];
} HttpRequest;

static bool str_eq(const char *s1, const char *s2) {
    return (strcmp(s1, s2) == 0) ? true : false;
}

// Server running flag
bool running = true;

// Hander for SIGINT
static void handle_sigint(int sig) {
    (void)sig;
    running = false;
}

static size_t copy_str_by(char *dst, const size_t dst_size, char *src,
                          char sep) {
    size_t i = 0;
    for (; i < (dst_size - 1); i++) {
        if (src[i] == sep) {
            break;
        }
        dst[i] = src[i];
    }
    dst[i] = '\0';

    return i;
}

// Get an HTTP request method from a received data
static char *get_request_method(RequestMethod *method, char *recv_buf) {
    char method_str[8];

    size_t n = copy_str_by(method_str, sizeof(method_str), recv_buf, ' ');

    if (str_eq(method_str, "GET")) {
        *method = REQUEST_GET;
    } else {
        *method = REQUEST_NONE;
    }

    return recv_buf + n + 1;
}

// Get URI from an HTTP request
static char *get_uri(char *uri_buf, const size_t uri_buf_size, char *recv_buf) {
    size_t n = copy_str_by(uri_buf, uri_buf_size, recv_buf, ' ');
    return recv_buf + n + 1;
}

// Get HTTP version
static char *get_version_str(char *ver_str_buf, const size_t ver_str_buf_size,
                             char *recv_buf) {
    size_t n = copy_str_by(ver_str_buf, ver_str_buf_size, recv_buf, '\n');

    return recv_buf + n + 1;
}

// Find a resource from URI
static bool find_resource(const char *uri, char *path_buf,
                          const size_t buf_size) {
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

HttpRequest parse_request(char *recv_buf) {
    char *p = recv_buf;

    HttpRequest request;
    p = get_request_method(&request.method, p);
    p = get_uri(request.uri, sizeof(request.uri), p);
    p = get_version_str(request.version, sizeof(request.version), p);

    return request;
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
                running = false;
            } else {
                PRINT_ERROR("accept() failed\n");
            }
            continue;
        }

        int recv_size = recv(conn_fd, recv_buf, BUF_SIZE, 0);
        if (recv_size == -1) {
            PRINT_ERROR("recv() failed\n");
            close(conn_fd);
            continue;
        }

        if (recv_size == 0) {
            PRINT_ERROR("Received data size is 0\n");
            close(conn_fd);
            continue;
        }

        HttpRequest request = parse_request(recv_buf);

        if (request.method == REQUEST_GET) {
            // Accept GET method
            if (str_eq(request.uri, "\0")) {
                snprintf(send_buf, BUF_SIZE, "HTTP/1.1 400 Bad Request\r\n");
            } else {
                char path[BUF_SIZE];
                if (find_resource(request.uri, path, sizeof(path))) {
                    create_response(send_buf, BUF_SIZE, path);
                } else {
                    snprintf(send_buf, BUF_SIZE, "HTTP/1.1 404 Not Found\r\n");
                }
            }
        } else {
            // Otherwise, return 405
            snprintf(send_buf, BUF_SIZE, "HTTP/1.1 405 Method Not Allowed\r\n");
        }

        int sent_size = send(conn_fd, send_buf, (BUF_SIZE + 1), 0);
        if (sent_size == -1) {
            PRINT_ERROR("send() failed\n");
            close(conn_fd);
            continue;
        }

        close(conn_fd);
    }

    close(sock_fd);
    PRINT_INFO("Connection closed\n");

    return EXIT_SUCCESS;
}
