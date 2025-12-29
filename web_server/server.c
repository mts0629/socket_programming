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
// Path to resources
static char resource_root[BUF_SIZE];

// Type of HTTP request method
typedef enum { REQUEST_NONE, REQUEST_GET } RequestMethod;

// MIME type
typedef enum { MIMETYPE_NONE, MIMETYPE_TEXT_HTML } MimeType;

// HTTP request
typedef struct {
    RequestMethod method;
    char uri[BUF_SIZE];
    char version[16];
} HttpRequest;

// HTTP response
typedef struct {
    int status_code;
    MimeType mime_type;
    char content[BUF_SIZE];
} HttpResponse;

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

// Copy string by specified separator
static size_t copy_str_by(char *dst, const size_t dst_size, const char *src,
                          const char sep) {
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

// Parse an HTTP request
HttpRequest parse_request(char *recv_buf) {
    char *p_buf = recv_buf;

    HttpRequest request;
    p_buf = get_request_method(&request.method, p_buf);
    p_buf = get_uri(request.uri, sizeof(request.uri), p_buf);
    p_buf = get_version_str(request.version, sizeof(request.version), p_buf);

    return request;
}

// Find a resource from URI
bool find_resource(char *path_buf, const size_t buf_size, const char *uri) {
    strncpy(path_buf, resource_root, buf_size);
    size_t len = strlen(path_buf);

    if (str_eq(uri, "/")) {
        strncpy((path_buf + len), "/index.html", 12);
    } else {
        strncpy((path_buf + len), uri, strlen(uri));
    }

    // Try to open a file to check its existence
    FILE *fp = fopen(path_buf, "r");
    if (fp == NULL) {
        return false;
    }
    fclose(fp);

    return true;
}

// Get MIME type
MimeType get_mime_type(char *file_path) {
    // Find an extension from a file path
    size_t i = strlen(file_path);
    while (file_path[i] != '.') {
        i--;
    }

    char *p = &file_path[i];
    if (str_eq(p, ".html")) {
        return MIMETYPE_TEXT_HTML;
    }

    return MIMETYPE_NONE;
}

// Get a file content
void get_content(char *buf, const size_t buf_size, const char *resource_path) {
    // Content of HTTP response
    memset(buf, '\0', buf_size);

    // Copy a file content
    FILE *fp = fopen(resource_path, "r");
    size_t i = 0;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        buf[i] = (char)c;
        i++;
        if (i == buf_size) {
            break;
        }
    }
    fclose(fp);
}

static size_t write_status_line(char *buf, const size_t buf_size,
                                const int status_code) {
    size_t n = 0;

    char *status_str = NULL;
    switch (status_code) {
        case 200:
            status_str = "OK";
            break;
        case 400:
            status_str = "Bad Request";
            break;
        case 404:
            status_str = "Not Found";
            break;
        case 405:
            status_str = "Method Not Allowed";
            break;
        default:
            break;
    }

    const char *version_str = "HTTP/1.1";
    if (status_str != NULL) {
        n = snprintf(buf, buf_size, "%s %d %s\r\n", version_str, status_code,
                     status_str);
    }

    return n;
}

static char *get_mime_type_str(const MimeType mime_type) {
    switch (mime_type) {
        case MIMETYPE_TEXT_HTML:
            return "text/html; charset=utf-8";
            break;
        default:
            break;
    }

    return "application/octet-stream";  // Ambiguous binary format
}

// Create an HTTP response
void write_response(char *buf, const size_t buf_size,
                    const HttpResponse *response) {
    char *p_buf = buf;
    size_t rem_size = buf_size;

    size_t n = write_status_line(p_buf, rem_size, response->status_code);
    p_buf += n;
    rem_size -= n;

    if (response->status_code == 200) {
        snprintf(p_buf, rem_size,
                 "Content-Type: %s\r\n"
                 "Content-Length: %lu\r\n"
                 "\r\n"
                 "%s",
                 get_mime_type_str(response->mime_type),
                 strlen(response->content), response->content);
    }
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

        HttpResponse response;
        if (request.method == REQUEST_GET) {
            // Accept GET method
            if (str_eq(request.uri, "\0")) {
                response.status_code = 400;
            } else {
                char path[BUF_SIZE];
                if (find_resource(path, sizeof(path), request.uri)) {
                    response.status_code = 200;
                    response.mime_type = get_mime_type(path);
                    get_content(response.content, sizeof(response.content),
                                path);
                } else {
                    response.status_code = 404;
                }
            }
        } else {
            // Otherwise, return 405
            response.status_code = 405;
        }

        write_response(send_buf, sizeof(send_buf), &response);

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
