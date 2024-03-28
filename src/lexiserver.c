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
#define BUFFER_SIZE 1024 // solo para los errores y si tambuffer no esta definido en la config. No tiene ningun efecto

// SI NO EDITARON LA CONFIG, TOMA ESTOS VALORES POR DEFECTO.
#define DEFAULT_PUERTO 1337
#define DEFAULT_WEB_ROOT "/tmp/www"
#define DEFAULT_TAMBUFFER 1024
#define DEFAULT_LEXISERVER "lexiserver-1.0.2" // No tiene un uso concreto ahora. Es mas un boilerplate que otra cosa en este momento.

// DEFINE TAMBUFFER COMO UNA CONSTANTE GLOBAL
const int TAMBUFFER = BUFFER_SIZE;

void parse_config_file(int *PUERTO, char *WEB_ROOT, int *TAMBUFFER, char *LEXISERVER) {
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
            if (strcmp(key, "PUERTO") == 0)
                *PUERTO = atoi(value);
            else if (strcmp(key, "WEB_ROOT") == 0)
                strcpy(WEB_ROOT, value);
            else if (strcmp(key, "TAMBUFFER") == 0)
                *TAMBUFFER = atoi(value);
            else if (strcmp(key, "LEXISERVER") == 0)
                strcpy(LEXISERVER, value);
        }
    }

    fclose(file);
}

void handle_request(int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int TAMBUFFER);
void send_file(int sockfd, const char *filepath, int TAMBUFFER);
void send_error(int sockfd, int status_code, const char *error_page);

int main() {
    int PUERTO, TAMBUFFER;
    char WEB_ROOT[256], LEXISERVER[256];
    parse_config_file(&PUERTO, WEB_ROOT, &TAMBUFFER, LEXISERVER);
// string con string, integrer con integrer y todos felices
    printf("* Configuracion en uso:\n* PUERTO=%d\n* WEB_ROOT=%s\n* TAMBUFFER=%d\n* LEXISERVER=%s\n", PUERTO, WEB_ROOT, TAMBUFFER, LEXISERVER);

    char buffer[TAMBUFFER];
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("* Socket creado [ OK ]\n* Server Escuchando\n* Lexiserver 1.0.2 https://alexia.lat/docs/lexiserver\n");

    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PUERTO);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }

    printf("* Bindeando IP al Socket [ OK ]\n");

    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }

    printf("* Server Escuchando conexiones en Puerto %d\n", PUERTO);

    for (;;) {
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        handle_request(newsockfd, client_addr, WEB_ROOT, TAMBUFFER);
    }

    return 0;
}

void handle_request(int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int TAMBUFFER) {
    char buffer[TAMBUFFER];
    int valread = read(newsockfd, buffer, TAMBUFFER);
    if (valread < 0) {
        perror("webserver (read)");
        close(newsockfd);
        return;
    }

    char method[TAMBUFFER], uri[TAMBUFFER], version[TAMBUFFER];
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), method, version, uri);

    char filepath[TAMBUFFER];
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

    send_file(newsockfd, filepath, TAMBUFFER);
    close(newsockfd);
}

void send_file(int sockfd, const char *filepath, int TAMBUFFER) {
    char buffer[TAMBUFFER];
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

    while ((file_size = read(filefd, buffer, TAMBUFFER)) > 0) {
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
