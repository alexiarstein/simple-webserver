// Lexiserver - A small webserver for tinkering and educational purposes written in C
// Author: Alexia Michelle <alexiarstein@aol.com
// COMPILE: gcc lexiserver.c -o lexiserver
// LICENSE: GNU GENERAL PUBLIC LICENSE 3.0 (SEE LICENSE FOR MORE INFO)
// WEB: https://alexia.lat/docs/lexiserver
// PROJECT PAGE: https://github.com/alexiarstein/simple-webserver
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CONFIG_FILE "lexiserver.conf"
#define BUFFER_SIZE 1024 // solo para los errores y si LBUFSIZE no esta definido en la config. No tiene ningun efecto

// SI NO EDITARON LA CONFIG, TOMA ESTOS VALORES POR DEFECTO.
#define DEFAULT_LPORT 1337
#define DEFAULT_WEB_ROOT "/tmp/www"
#define DEFAULT_LBUFSIZE 1024
#define DEFAULT_LEXISERVER "lexiserver-1.1.0" // No tiene un uso concreto ahora. Es mas un boilerplate que otra cosa en este momento.

// DEFINE LBUFSIZE COMO UNA CONSTANTE GLOBAL
const int LBUFSIZE = BUFFER_SIZE;

void parse_config_file(int *LPORT, char *WEB_ROOT, int *LBUFSIZE, char *LEXISERVER) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("ERROR OPENING CONFIG. FILE"); // SI NO HAY lexiserver.conf en el mismo lugar que el binario, putea.
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2) {
            if (strcmp(key, "LPORT") == 0)
                *LPORT = atoi(value);
            else if (strcmp(key, "WEB_ROOT") == 0)
                strcpy(WEB_ROOT, value);
            else if (strcmp(key, "LBUFSIZE") == 0)
                *LBUFSIZE = atoi(value);
            else if (strcmp(key, "LEXISERVER") == 0)
                strcpy(LEXISERVER, value);
        }
    }

    fclose(file);
}

void init_openssl() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method(); // Use SSLv23_server_method for compatibility
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    // Set the certificate file and key file
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void handle_request(SSL *ssl, const char *WEB_ROOT, int LBUFSIZE);
void send_file(SSL *ssl, const char *filepath, int LBUFSIZE);
void send_error(SSL *ssl, int status_code, const char *error_page);

int main() {
    int LPORT, LBUFSIZE;
    char WEB_ROOT[256], LEXISERVER[256];
    parse_config_file(&LPORT, WEB_ROOT, &LBUFSIZE, LEXISERVER);

    printf("* Configuration Loaded:\n* PORT=%d\n* WEB_ROOT=%s\n* BUFFER SIZE=%d\n* LEXISERVER=%s\n", LPORT, WEB_ROOT, LBUFSIZE, LEXISERVER);

    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("* Socket Created [ OK ]\n* Server Listening\n* Lexiserver 1.0.2 https://alexia.lat/docs/lexiserver\n");

    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(LPORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }

    printf("* Bound Socket to Address [ OK ]\n");

    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }

    printf("* Server Listening Connections on Port %d\n", LPORT);

    for (;;) {
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, newsockfd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            handle_request(ssl, WEB_ROOT, LBUFSIZE);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(newsockfd);
    }

    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}

// Definition of handle_request function
void handle_request(SSL *ssl, const char *WEB_ROOT, int LBUFSIZE) {
    char buffer[LBUFSIZE];
    int valread = SSL_read(ssl, buffer, LBUFSIZE);
    if (valread < 0) {
        perror("webserver (read)");
        return;
    }

    char method[LBUFSIZE], uri[LBUFSIZE], version[LBUFSIZE];
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("[SSL Connection] %s %s %s\n", method, version, uri);

    char filepath[LBUFSIZE];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, uri);
    if (strlen(uri) == 1 && uri[0] == '/') {
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
    }

    int filefd = open(filepath, O_RDONLY);
    if (filefd < 0) {
        perror("webserver (open)");
        send_error(ssl, 404, "404.html");
        return;
    }

    send_file(ssl, filepath, LBUFSIZE);
    close(filefd);
}

// Definition of send_file function
void send_file(SSL *ssl, const char *filepath, int LBUFSIZE) {
    char buffer[LBUFSIZE];
    ssize_t file_size;
    int valwrite;

    int filefd = open(filepath, O_RDONLY);
    if (filefd < 0) {
        perror("webserver (open file)");
        return;
    }

    char response_headers[] = "HTTP/1.0 200 OK\r\n"
                              "Server: Lexiserver 1.0.2\r\n"
                              "Content-type: text/html\r\n\r\n";
    valwrite = SSL_write(ssl, response_headers, strlen(response_headers));
    if (valwrite < 0) {
        perror("webserver (write headers)");
        close(filefd);
        return;
    }

    while ((file_size = read(filefd, buffer, LBUFSIZE)) > 0) {
        valwrite = SSL_write(ssl, buffer, file_size);
        if (valwrite < 0) {
            perror("webserver (write file)");
            close(filefd);
            return;
        }
    }

    close(filefd);
}

// Definition of send_error function
void send_error(SSL *ssl, int status_code, const char *error_page) {
    char error_message[256];
    snprintf(error_message, sizeof(error_message), "HTTP/1.0 %d ERROR\r\n\r\n", status_code);
    SSL_write(ssl, error_message, strlen(error_message));

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    FILE *file = fopen(error_page, "r");
    if (file == NULL) {
        perror("ERROR OPENING ERROR FILE");
        return;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (SSL_write(ssl, buffer, bytes_read) < 0) {
            perror("ERROR SENDING ERROR FILE TO CLIENT");
            fclose(file);
            return;
        }
    }

    fclose(file);
}