// Lexiserver - A small webserver for tinkering and educational purposes written in C
// Author: Alexia Michelle <alexiarstein@aol.com
// COMPILE: gcc lexiserver.c -o lexiserver
// LICENSE: GNU GENERAL PUBLIC LICENSE 3.0 (SEE LICENSE FOR MORE INFO)
// WEB: https://alexia.lat/docs/lexiserver
// PROJECT PAGE: https://github.com/alexiarstein/simple-webserver

#include "lexiserver.h"

// DEFINE LBUFSIZE COMO UNA CONSTANTE GLOBAL
const int LBUFSIZE = BUFFER_SIZE;

int parse_config_file(struct cfg_struct* config){//int *LPORT, char *WEB_ROOT, int *LBUFSIZE, char *LEXISERVER) {
    int ret_val = 1;

    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("ERROR OPENING CONFIG. FILE"); // SI NO HAY lexiserver.conf en el mismo lugar que el binario, putea.
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2) {
            if (strcmp(key, "LPORT") == 0){
                if (int_check(value) == 0) {
                    printf("ERROR PARSING CONFIG. LPORT MUST BE AN INTEGER.\n");
                    ret_val = 0;
                    break;
                }
                config->LPORT = atoi(value);
            }
                
            else if (strcmp(key, "WEB_ROOT") == 0){
                if (access(value, F_OK) == -1) {
                    printf("WARNING: WEB_ROOT path '%s' does not exist. Defaulting to '%s'.\n", value, DEFAULT_WEB_ROOT);
                    strcpy(config->WEB_ROOT, DEFAULT_WEB_ROOT);
                } else {
                    strcpy(config->WEB_ROOT, value);
                }
            }
                
            else if (strcmp(key, "LBUFSIZE") == 0){
                if (int_check(value) == 0) {
                    printf("ERROR PARSING CONFIG. LBUFSIZE MUST BE AN INTEGER.\n");
                    ret_val = 0;
                    break;
                }
                config->LBUFSIZE = atoi(value);
            }

            else if (strcmp(key, "LEXISERVER") == 0)
                strcpy(config->LEXISERVER, value);
            
            else if (strcmp(key, "SSL_CERT") == 0)
                strcpy(config->SSL_CERT, value);

            else if (strcmp(key, "SSL_KEY") == 0)
                strcpy(config->SSL_KEY, value);

            else if (strcmp(key, "SSL_WEB_ROOT") == 0){
                if (access(value, F_OK) == -1) {
                    printf("WARNING: SSL_WEB_ROOT path '%s' does not exist. Defaulting to '%s'.\n", value, DEFAULT_WEB_ROOT);
                    strcpy(config->SSL_WEB_ROOT, DEFAULT_WEB_ROOT);
                } else {
                    strcpy(config->SSL_WEB_ROOT, value);
                }
            }

            else if (strcmp(key, "SSL_PORT") == 0){
                if (int_check(value) == 0) {
                    printf("ERROR PARSING CONFIG. SSL_PORT MUST BE AN INTEGER.\n");
                    ret_val = 0;
                    break;
                }
                config->SSL_PORT = atoi(value);
            }
        }
    }
    if (strlen(config->SSL_CERT) > 0 && strlen(config->SSL_KEY) > 0 && strlen(config->SSL_WEB_ROOT) > 0 && config->SSL_PORT > 0) {
        strcpy(config->USE_SSL, "Y");
    }
    fclose(file);
    return ret_val;
}

int int_check(char *value) {
    for (int i = 0; i < strlen(value); i++) {
        if (isdigit(value[i]) == 0)
            return 0;
    }
    return 1;
}

void init_openssl(void) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

SSL_CTX *create_context(void) {
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

void configure_context(SSL_CTX *ctx, struct cfg_struct config) {
    // Set the certificate file and key file
    if (SSL_CTX_use_certificate_file(ctx, config.SSL_CERT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, config.SSL_KEY, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    struct cfg_struct config;
    int ssl_child_pid;
    SSL_CTX *ctx;
    struct sockaddr_in host_addr;
    struct sockaddr_in client_addr;
    int sockfd;

    if (!parse_config_file(&config)) {
        printf("ERROR LOADING CONFIG. SERVER NOT STARTING.\n");
        return 1;
    }
    
    // string con string, integrer con integrer y todos felices
    printf("* Configuration Loaded:\n----------\n* PORT=%d\n* WEB_ROOT=%s\n* BUFFER SIZE=%d\n* LEXISERVER=%s\n", config.LPORT, config.WEB_ROOT, config.LBUFSIZE, config.LEXISERVER);

    if (strcmp(config.USE_SSL, "Y") == 0) {
        printf("* SSL Enabled\n* SSL port: %d\n* SSL WEB_ROOT: %s\n----------\n", config.SSL_PORT, config.SSL_WEB_ROOT);
        init_openssl();
        ctx = create_context();
        configure_context(ctx, config);
        // spawn new process to handle SSL connections
        ssl_child_pid = fork();
        // if we get a PID, then reconfigure this parent process to use unsecure connection
        if (ssl_child_pid > 0){
            strcpy(config.USE_SSL, "N");
        }
    }
    else{
        printf("* SSL NOT Enabled\n----------\n");
        ssl_child_pid = 0;
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("* Socket Created [ OK ]\n* Server Listening\n* Lexiserver 1.0.2 https://alexia.lat/docs/lexiserver\n");

    int host_addrlen = sizeof(host_addr);
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(strcmp(config.USE_SSL, "Y") == 0 ? config.SSL_PORT : config.LPORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }

    printf("* Bound Socket to Address [ OK ]\n");

    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }

    printf("%s Server Listening Connections on Port %d\n", strcmp(config.USE_SSL, "Y") == 0 ? "* Secure" : "*", strcmp(config.USE_SSL, "Y") == 0 ? config.SSL_PORT : config.LPORT);

    for (;;) {
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");
        if (strcmp(config.USE_SSL, "Y") == 0) {
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, newsockfd);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
            } else {
                handle_request(ssl, 0, client_addr, config.SSL_WEB_ROOT, config.LBUFSIZE);
            }

            SSL_shutdown(ssl);
            SSL_free(ssl);
        } else {
            handle_request(NULL, newsockfd, client_addr, config.WEB_ROOT, config.LBUFSIZE);
            close(newsockfd);
        }
    }

    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}

void handle_request(SSL *ssl, int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int LBUFSIZE) {
    char buffer[LBUFSIZE];
    int valread;

    if (newsockfd > 0) {
        valread = read(newsockfd, buffer, LBUFSIZE);
    } else {
        valread = SSL_read(ssl, buffer, LBUFSIZE);
    }

    if (valread < 0) {
        perror("webserver (read)");
        if (newsockfd > 0) {
            close(newsockfd);
        }

        return;
    }

    char method[LBUFSIZE], uri[LBUFSIZE], version[LBUFSIZE];
    sscanf(buffer, "%s %s %s", method, uri, version);
    if (newsockfd > 0) {
        printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), method, version, uri);
    } else {
        printf("[SSL Connection] %s %s %s\n", method, version, uri);
    }

    char filepath[LBUFSIZE];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, uri);
    if (strlen(uri) == 1 && uri[0] == '/') {
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
    }

    int filefd = open(filepath, O_RDONLY);
    if (filefd < 0) {
        perror("webserver (open)");
        if (newsockfd > 0) {
            send_error(NULL, newsockfd, 404, "404.html");
            close(newsockfd);
        } else {
            send_error(ssl, 0, 404, "404.html");
        }
        return;
    }

    if (newsockfd > 0) {
        send_file(NULL, newsockfd, filepath, LBUFSIZE);
        close(newsockfd);
    } else {
        send_file(ssl, 0, filepath, LBUFSIZE);
    }
}

void send_file(SSL *ssl, int sockfd, const char *filepath, int LBUFSIZE) {
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
    
    if(sockfd > 0) {
        valwrite = write(sockfd, response_headers, strlen(response_headers));
    } else {
        valwrite = SSL_write(ssl, response_headers, strlen(response_headers));
    }

    if (valwrite < 0) {
        perror("webserver (write headers)");
        close(filefd);
        return;
    }

    while ((file_size = read(filefd, buffer, LBUFSIZE)) > 0) {
        if(sockfd > 0) {
            valwrite = write(sockfd, buffer, file_size);
        } else {
            valwrite = SSL_write(ssl, buffer, file_size);
        }
        if (valwrite < 0) {
            perror("webserver (write file)");
            close(filefd);
            return;
        }
    }

    close(filefd);
}

void send_error(SSL *ssl, int sockfd, int status_code, const char *error_page) {
    char error_message[256];
    snprintf(error_message, sizeof(error_message), "HTTP/1.0 %d ERROR\r\n\r\n", status_code);
    if (sockfd > 0) {
        write(sockfd, error_message, strlen(error_message));
    } else {
        SSL_write(ssl, error_message, strlen(error_message));
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    FILE *file = fopen(error_page, "r");
    if (file == NULL) {
        perror("ERROR OPENING ERROR FILE");
        return;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        int bytes;
        if (sockfd > 0) {
            bytes = write(sockfd, buffer, bytes_read);
        } else {
            bytes = SSL_write(ssl, buffer, bytes_read);
        }
        if (bytes < 0) {
            perror("ERROR SENDING ERROR FILE TO CLIENT");
            fclose(file);
            return;
        }
    }

    fclose(file);
}
