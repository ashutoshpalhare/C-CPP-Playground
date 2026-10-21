/*
 * Basic HTTP Server (C11)
 *
 * A deliberately small, single-threaded HTTP/1.1 static file server for
 * learning how sockets and the HTTP request/response cycle work.
 *   - Supports GET and HEAD only.
 *   - Serves files from a document root directory (default: ./public).
 *   - Listens on 127.0.0.1 only, so it is not reachable from other machines.
 *   - Works with Winsock on Windows and BSD sockets on Linux/macOS.
 *
 * It is NOT a production server: no TLS, no keep-alive, no threading, no
 * percent-decoding, and only one client is served at a time.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define INVALID_SOCK INVALID_SOCKET
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCK (-1)
#define CLOSE_SOCKET close
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#define DEFAULT_PORT 8080
#define DEFAULT_ROOT "public"
#define REQUEST_CAP 8192
#define PATH_CAP 1024
#define CHUNK 4096
#define MAX_FILE_BYTES (512UL * 1024UL * 1024UL)
#define RECV_TIMEOUT_SECONDS 5

/* ---------- platform helpers ---------- */

static int net_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    signal(SIGPIPE, SIG_IGN); /* a client closing early must not kill the server */
    return 1;
#endif
}

static void net_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void print_net_error(const char *what) {
#ifdef _WIN32
    fprintf(stderr, "%s failed (Winsock error %d)\n", what, WSAGetLastError());
#else
    fprintf(stderr, "%s failed: %s\n", what, strerror(errno));
#endif
}

static void set_recv_timeout(socket_t s, int seconds) {
#ifdef _WIN32
    DWORD ms = (DWORD)seconds * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms, sizeof ms);
#else
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
#endif
}

/* Sends the whole buffer, handling partial sends. Returns 1 on success. */
static int send_all(socket_t s, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(s, data + sent, (int)(len - sent), 0);
#else
        ssize_t n = send(s, data + sent, len - sent, MSG_NOSIGNAL);
#endif
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

/* ---------- HTTP helpers ---------- */

static int ends_with_ci(const char *text, const char *suffix) {
    size_t tl = strlen(text), sl = strlen(suffix);
    if (sl > tl) return 0;
    text += tl - sl;
    for (size_t i = 0; i < sl; i++) {
        char a = text[i], b = suffix[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (a != b) return 0;
    }
    return 1;
}

static const char *mime_type(const char *path) {
    static const struct { const char *ext; const char *type; } table[] = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "text/javascript; charset=utf-8"},
        {".json", "application/json"},
        {".txt", "text/plain; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
    };
    for (size_t i = 0; i < sizeof table / sizeof table[0]; i++) {
        if (ends_with_ci(path, table[i].ext)) return table[i].type;
    }
    return "application/octet-stream";
}

static void send_error(socket_t client, int code, const char *reason, const char *extra_headers, int head_only) {
    char body[256];
    char header[512];
    int body_len = snprintf(body, sizeof body,
                            "<!doctype html><html><head><title>%d %s</title></head>"
                            "<body><h1>%d %s</h1></body></html>\n",
                            code, reason, code, reason);
    int header_len = snprintf(header, sizeof header,
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: text/html; charset=utf-8\r\n"
                              "Content-Length: %d\r\n"
                              "Connection: close\r\n"
                              "X-Content-Type-Options: nosniff\r\n"
                              "%s\r\n",
                              code, reason, body_len, extra_headers ? extra_headers : "");
    if (header_len > 0 && header_len < (int)sizeof header) send_all(client, header, (size_t)header_len);
    if (!head_only) send_all(client, body, (size_t)body_len);
}

/* Accepts only a conservative set of characters and rejects ".." anywhere. */
static int safe_request_path(const char *path) {
    if (path[0] != '/') return 0;
    if (strstr(path, "..") != NULL) return 0;
    for (const char *p = path; *p; p++) {
        char c = *p;
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                 c == '/' || c == '.' || c == '_' || c == '-';
        if (!ok) return 0;
    }
    return 1;
}

static void serve_file(socket_t client, const char *fs_path, int head_only, int *status) {
    struct stat st;
    if (stat(fs_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        *status = 404;
        send_error(client, 404, "Not Found", NULL, head_only);
        return;
    }
    if ((unsigned long)st.st_size > MAX_FILE_BYTES) {
        *status = 500;
        send_error(client, 500, "Internal Server Error", NULL, head_only);
        return;
    }
    FILE *fp = fopen(fs_path, "rb");
    if (!fp) {
        *status = 403;
        send_error(client, 403, "Forbidden", NULL, head_only);
        return;
    }
    char header[512];
    int n = snprintf(header, sizeof header,
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %lu\r\n"
                     "Connection: close\r\n"
                     "X-Content-Type-Options: nosniff\r\n"
                     "\r\n",
                     mime_type(fs_path), (unsigned long)st.st_size);
    *status = 200;
    if (n > 0 && n < (int)sizeof header && send_all(client, header, (size_t)n) && !head_only) {
        char chunk[CHUNK];
        size_t got;
        while ((got = fread(chunk, 1, sizeof chunk, fp)) > 0) {
            if (!send_all(client, chunk, got)) break;
        }
    }
    fclose(fp);
}

static void handle_client(socket_t client, const char *root) {
    char request[REQUEST_CAP];
    size_t used = 0;

    /* Read until the blank line that ends the request headers. */
    while (used < sizeof request - 1) {
#ifdef _WIN32
        int n = recv(client, request + used, (int)(sizeof request - 1 - used), 0);
#else
        ssize_t n = recv(client, request + used, sizeof request - 1 - used, 0);
#endif
        if (n <= 0) break;
        used += (size_t)n;
        request[used] = '\0';
        if (strstr(request, "\r\n\r\n") != NULL) break;
    }
    request[used] = '\0';
    if (used == 0) return;

    char method[8], target[PATH_CAP], version[16];
    if (strstr(request, "\r\n") == NULL ||
        sscanf(request, "%7s %1023s %15s", method, target, version) != 3 ||
        strncmp(version, "HTTP/", 5) != 0) {
        printf("(malformed request) -> 400\n");
        send_error(client, 400, "Bad Request", NULL, 0);
        return;
    }

    int head_only = strcmp(method, "HEAD") == 0;
    if (!head_only && strcmp(method, "GET") != 0) {
        printf("%s (unsupported method) -> 405\n", method);
        send_error(client, 405, "Method Not Allowed", "Allow: GET, HEAD\r\n", 0);
        return;
    }

    /* Drop any query string or fragment; they are ignored by this server. */
    char *cut = strpbrk(target, "?#");
    if (cut) *cut = '\0';

    if (!safe_request_path(target)) {
        printf("%s (rejected path) -> 400\n", method);
        send_error(client, 400, "Bad Request", NULL, head_only);
        return;
    }

    char fs_path[PATH_CAP + 256];
    const char *index_name = target[strlen(target) - 1] == '/' ? "index.html" : "";
    int written = snprintf(fs_path, sizeof fs_path, "%s%s%s", root, target, index_name);
    if (written < 0 || (size_t)written >= sizeof fs_path) {
        send_error(client, 400, "Bad Request", NULL, head_only);
        return;
    }

    int status = 0;
    serve_file(client, fs_path, head_only, &status);
    printf("%s %s -> %d\n", method, target, status);
}

/* ---------- main ---------- */

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    const char *root = DEFAULT_ROOT;

    if (argc > 3) {
        fprintf(stderr, "Usage: %s [port] [document_root]\n", argv[0]);
        return 1;
    }
    if (argc >= 2) {
        char *end;
        long p = strtol(argv[1], &end, 10);
        if (*argv[1] == '\0' || *end != '\0' || p < 1 || p > 65535) {
            fprintf(stderr, "Invalid port \"%s\". Use a number from 1 to 65535.\n", argv[1]);
            return 1;
        }
        port = (int)p;
    }
    if (argc == 3) root = argv[2];

    struct stat root_stat;
    if (stat(root, &root_stat) != 0 || !S_ISDIR(root_stat.st_mode)) {
        fprintf(stderr, "Document root \"%s\" does not exist or is not a directory.\n", root);
        fprintf(stderr, "Run the server from this project folder, or pass the folder as the 2nd argument.\n");
        return 1;
    }

    if (!net_init()) {
        fprintf(stderr, "Could not initialise the network library.\n");
        return 1;
    }

    socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCK) {
        print_net_error("socket");
        net_cleanup();
        return 1;
    }
#ifndef _WIN32
    /* Lets the server restart immediately. (On Windows this option has different, unsafe semantics.) */
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof addr) != 0) {
        print_net_error("bind (is the port already in use?)");
        CLOSE_SOCKET(listener);
        net_cleanup();
        return 1;
    }
    if (listen(listener, 16) != 0) {
        print_net_error("listen");
        CLOSE_SOCKET(listener);
        net_cleanup();
        return 1;
    }

    printf("Serving \"%s\" at http://127.0.0.1:%d/  (press Ctrl+C to stop)\n", root, port);

    for (;;) {
        socket_t client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCK) {
            print_net_error("accept");
            continue;
        }
        set_recv_timeout(client, RECV_TIMEOUT_SECONDS);
        handle_client(client, root);
        CLOSE_SOCKET(client);
    }

    /* Not reached: the server runs until interrupted. */
    CLOSE_SOCKET(listener);
    net_cleanup();
    return 0;
}
