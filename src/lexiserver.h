#ifndef LEXISERVER_H
#  define LEXISERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
#define DEFAULT_LEXISERVER "lexiserver-1.0.2" // No tiene un uso concreto ahora. Es mas un boilerplate que otra cosa en este momento.

struct cfg_struct {
    int LPORT;
    char WEB_ROOT[256];
    int LBUFSIZE;
    char LEXISERVER[256];
    char USE_SSL[2];
    char SSL_CERT[256];
    char SSL_KEY[256];
    char SSL_WEB_ROOT[256];
    int SSL_PORT;
};

void handle_request(SSL *ssl, int newsockfd, struct sockaddr_in client_addr, const char *WEB_ROOT, int LBUFSIZE);
void send_file(SSL *ssl, int sockfd, const char *filepath, int LBUFSIZE);
void send_error(SSL *ssl, int sockfd, int status_code, const char *error_page);
int int_check(char *value);
void init_openssl(void);
SSL_CTX *create_context(void);
void configure_context(SSL_CTX *ctx, struct cfg_struct config);

#endif

