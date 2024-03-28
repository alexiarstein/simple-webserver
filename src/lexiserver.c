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

#define CONFIG_FILE "lexiserver.conf"
#define BUFFER_SIZE 1024 // solo para los errores y si LBUFSIZE no esta definido en la config. No tiene ningun efecto

// SI NO EDITARON LA CONFIG, TOMA ESTOS VALORES POR DEFECTO.
#define DEFAULT_LPORT 1337
#define DEFAULT_WEB_ROOT "/tmp/www"
#define DEFAULT_LBUFSIZE 1024
#define DEFAULT_LEXISERVER "lexiserver-1.0.2" // No tiene un uso concreto ahora. Es mas un boilerplate que otra cosa en este momento.

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

void handle_request(int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int LBUFSIZE);
void send_file(int sockfd, const char *filepath, int LBUFSIZE);
void send_error(int sockfd, int status_code, const char *error_page);

int main() {
    int LPORT, LBUFSIZE;
    char WEB_ROOT[256], LEXISERVER[256];
    parse_config_file(&LPORT, WEB_ROOT, &LBUFSIZE, LEXISERVER);
// string con string, integrer con integrer y todos felices
    printf("* Configuration Loaded:\n* PORT=%d\n* WEB_ROOT=%s\n* BUFFER SIZE=%d\n* LEXISERVER=%s\n", LPORT, WEB_ROOT, LBUFSIZE, LEXISERVER);

    char buffer[LBUFSIZE];
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
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        handle_request(newsockfd, client_addr, WEB_ROOT, LBUFSIZE);
    }

    return 0;
}

void handle_request(int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int LBUFSIZE) {
    char buffer[LBUFSIZE];
    int valread = read(newsockfd, buffer, LBUFSIZE);
    if (valread < 0) {
        perror("webserver (read)");
        close(newsockfd);
        return;
    }

    char method[LBUFSIZE], uri[LBUFSIZE], version[LBUFSIZE];
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), method, version, uri);

    char filepath[LBUFSIZE];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, uri);
    if (strlen(uri) == 1 && uri[0] == '/') {
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
    }

    int filefd = open(filepath, O_RDONLY);
    if (filefd < 0) {
        perror("webserver (open)");
        send_error(newsockfd, 404, "404.html");
        close(newsockfd);
        return;
    }

    send_file(newsockfd, filepath, LBUFSIZE);
    close(newsockfd);
}

void send_file(int sockfd, const char *filepath, int LBUFSIZE) {
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
    valwrite = write(sockfd, response_headers, strlen(response_headers));
    if (valwrite < 0) {
        perror("webserver (write headers)");
        close(filefd);
        return;
    }

    while ((file_size = read(filefd, buffer, LBUFSIZE)) > 0) {
        valwrite = write(sockfd, buffer, file_size);
        if (valwrite < 0) {
            perror("webserver (write file)");
            close(filefd);
            return;
        }
    }

    close(filefd);
}

void send_error(int sockfd, int status_code, const char *error_page) {
    char error_message[256];
    snprintf(error_message, sizeof(error_message), "HTTP/1.0 %d ERROR\r\n\r\n", status_code);
    write(sockfd, error_message, strlen(error_message));

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    FILE *file = fopen(error_page, "r");
    if (file == NULL) {
        perror("ERROR OPENING ERROR FILE");
        return;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (write(sockfd, buffer, bytes_read) < 0) {
            perror("ERROR SENDING ERROR FILE TO CLIENT");
            fclose(file);
            return;
        }
    }

    fclose(file);
}
