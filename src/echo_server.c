#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int sock = -1, client_sock = -1;
char buf[BUF_SIZE];

// Define HTTP response templates
char tmp_400[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
char tmp_501[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";

// Function to close a socket and handle errors
int close_socket(int sock) {
    if (close(sock)) {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

// Signal handler
void handle_signal(const int sig) {
    if (sock != -1) {
        fprintf(stderr, "\nReceived signal %d. Closing socket.\n", sig);
        close_socket(sock);
    }
    exit(0);
}

// SIGPIPE handler
void handle_sigpipe(const int sig) {
    if (sock != -1) {
        return;
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    /* Register signal handlers */
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGSEGV, handle_signal);
    signal(SIGABRT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTSTP, handle_signal);
    signal(SIGFPE, handle_signal);
    signal(SIGHUP, handle_signal);
    signal(SIGPIPE, handle_sigpipe);

    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;

    fprintf(stdout, "----- Echo Server -----\n");

    /* Create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    /* Set socket options */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fprintf(stderr, "Failed setting socket options.\n");
        return EXIT_FAILURE;
    }

    /* Bind the socket to a port */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    /* Listen for incoming connections */
    if (listen(sock, 5)) {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    while (1) {
        /* Wait for a new connection */
        cli_size = sizeof(cli_addr);
        fprintf(stdout, "Waiting for connection...\n");
        client_sock = accept(sock, (struct sockaddr *) &cli_addr, &cli_size);

        if (client_sock == -1) {
            fprintf(stderr, "Error accepting connection.\n");
            close_socket(sock);
            return EXIT_FAILURE;
        }

        fprintf(stdout, "New connection from %s:%d\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        
        ssize_t readret;
        while ((readret = recv(client_sock, buf, BUF_SIZE, 0)) > 0) {
            int valid_request = 0;
            int method_supported = 0;
            char *crlf = strstr(buf, "\r\n");
            char *method_end = NULL;

            if (crlf) {
                method_end = strchr(buf, ' ');
                if (method_end && method_end < crlf) {
                    size_t method_len = method_end - buf;
                    char method[16] = {0};
                    strncpy(method, buf, method_len);
                    method[method_len] = '\0';
                    valid_request = 1;

                    if (strcmp(method, "GET") == 0 || 
                        strcmp(method, "POST") == 0 || 
                        strcmp(method, "HEAD") == 0) {
                        method_supported = 1;
                    }
                }
            }

            if (!valid_request) {
                if (send(client_sock, tmp_400, strlen(tmp_400), 0) < 0) {
                    close_socket(client_sock);
                    close_socket(sock);
                    fprintf(stderr, "Error sending to client.\n");
                    return EXIT_FAILURE;
                }
            } else if (!method_supported) {
                if (send(client_sock, tmp_501, strlen(tmp_501), 0) < 0) {
                    close_socket(client_sock);
                    close_socket(sock);
                    fprintf(stderr, "Error sending to client.\n");
                    return EXIT_FAILURE;
                }
            } else {
                if (send(client_sock, buf, readret, 0) != readret) {
                    close_socket(client_sock);
                    close_socket(sock);
                    fprintf(stderr, "Error sending to client.\n");
                    return EXIT_FAILURE;
                }
            }
            memset(buf, 0, BUF_SIZE);
        }

        if (readret == -1) {
            fprintf(stderr, "Error reading from client.\n");
        }

        if (close_socket(client_sock)) {
            close_socket(sock);
            fprintf(stderr, "Error closing client socket.\n");
            return EXIT_FAILURE;
        }

        fprintf(stdout, "Closed connection from %s:%d\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    }

    close_socket(sock);
    return EXIT_SUCCESS;
}
