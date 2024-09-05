#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <ncurses.h>
#include <pwd.h>

#include "mytalk.h"


int main(int argc, char *argv[]) {
    MyTalkConfig config;
    int opt;
    /* Initialize settings */
    config.verbosity = 0;
    config.accept_all = 0;
    config.no_windowing = 0;
    config.hostname = NULL;
    config.port = 0;

    /* Grab flags set */
    while ((opt = getopt(argc, argv, "vaN")) != -1) {
        switch (opt) {
            case 'v':
                config.verbosity++;
                break;
            case 'a':
                config.accept_all = 1;
                break;
            case 'N':
                config.no_windowing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-v] [-a] [-N] [hostname] port\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* Check if there is any arguments after flags */
    if (optind < argc) {
        /* If only 1 argument left, must be port number */
        if (argc - optind == 1) {
            config.port = atoi(argv[optind]);
        /* Else if 2 arguments, it must be hostname then port */
        } else if (argc - optind == 2) {
            config.hostname = argv[optind];
            config.port = atoi(argv[optind + 1]);
        /* Else print usage */
        } else {
            fprintf(stderr, "Usage: %s [-v] [-a] [-N] [hostname] port\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    /* If no arugments are given, print error */
    } else {
        fprintf(stderr, "Port number is required.\n");
        exit(EXIT_FAILURE);
    }

    /* apply verbosity */
    set_verbosity(config.verbosity);

    /* Check whether we are the server or client */
    if (config.hostname) {
        /*fprintf(stderr, "Starting client\n");*/
        setup_client(&config);
    } else {
        /*fprintf(stderr, "Starting server\n");*/
        setup_server(&config);
    }

    return 0;
}

/* This function sets up the server */
void setup_server(MyTalkConfig *config) {
    struct addrinfo hints, *res;
    int server_fd;
    int opt = 1;
    char port_str[6];

    /* Put port into a buffer */
    snprintf(port_str, sizeof(port_str), "%d", config->port);
    /* Set signal handler */
    setup_signal_handler(handle_sigint);

    /* Clear hints */
    memset(&hints, 0, sizeof(hints));
    /* Set hints values to TCP */
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /* Get address information of port */
    if (getaddrinfo(NULL, port_str, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    /* Create socket */
    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd == -1) {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    /* Setup socket */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    /* Bind socket */
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    /* Free res */
    freeaddrinfo(res);

    /* Try to listen */
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Update server_socket */
    config->server_socket = server_fd;

    /* Try to connect */
    handle_server_connection(config);

    /* Start the chat with client */
    start_chat(config);

    /* Close server */
    close(server_fd);
}

/* This function handles the setup for client */
void setup_client(MyTalkConfig *config) {
    struct addrinfo hints, *res;
    int sockfd;
    char port_str[6];

    /* Start signal handler */
    setup_signal_handler(handle_sigint);

    /* Put port into buffer */
    snprintf(port_str, sizeof(port_str), "%d", config->port);

    /* Clear hints */
    memset(&hints, 0, sizeof(hints));
    /* Set TCP values */
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Get address info of the hostname with port */
    if (getaddrinfo(config->hostname, port_str, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    /* Create socket */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    /* Try to connect */
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    /* free res */
    freeaddrinfo(res);

    /* Change client_socket */
    config->client_socket = sockfd;

    /* Try to connect to server */
    handle_client_connection(config);

    /* Start chat */
    start_chat(config);

    /* Close socket */
    close(sockfd);
}

/* This function handles the connection to client from server */
void handle_server_connection(MyTalkConfig *config) {
    struct sockaddr client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[256];
    char response;
    int n;

    /* Attemp to accept the connection to
     client change client_socket to that value */
    config->client_socket = accept(config->server_socket,
        (struct sockaddr*)&client_addr, &client_len);
    if (config->client_socket < 0) {
        perror("Accept failed");
        close(config->server_socket);
        exit(EXIT_FAILURE);
    }

    /* Store username from client in buffer */
    n = recv(config->client_socket, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("Error reading from socket");
        close(config->client_socket);
        close(config->server_socket);
        exit(EXIT_FAILURE);
    }
    /* Nul terminate it */
    buffer[n] = '\0';


    /* If we set -a flag, skip this and accept connection */
    if (config->accept_all) {
        response = 'y';
    } else {
        /* Print request message with username */
        fprintf(stdout, "Mytalk request from %s. Accept (y/n)? ", buffer);
        fflush(stdout);
        /* Ask server for yes or no */
        scanf(" %c", &response);
    }

    /* If response was yes */
    if (response == 'y' || response == 'Y') {
        /* Send ok message */
        send(config->client_socket, "ok", 2, 0);
        
    } else {
        /* Else send no message */
        send(config->client_socket, "no", 2, 0);
        /* Close sockets */
        close(config->client_socket);
        close(config->server_socket);
        exit(EXIT_SUCCESS);
    }
}

/* This function handles connecting to server from client */
void handle_client_connection(MyTalkConfig *config) {
    char buffer[256];
    int n;
    struct passwd *password;

    /* Grab user information */
    password = getpwuid(getuid());

    /* Send username along with client socket
        information to server */
    send(config->client_socket, password->pw_name,
            strlen(password->pw_name), 0);

    /* Print waiting for response from hostname */
    printf("Waiting for response from %s\n", config->hostname);

    /* Store response from server into buffer */
    n = recv(config->client_socket, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("Error reading from socket");
        close(config->client_socket);
        exit(EXIT_FAILURE);
    }
    /* Nul terminate buffer */
    buffer[n] = '\0';

    /* If not ok message, exit */
    if (strcmp(buffer, "ok") != 0) {
        printf("%s declined connection.\n", config->hostname);
        close(config->client_socket);
        exit(EXIT_SUCCESS);
    }
}

/* This function handles the chat for both client and server */
void start_chat(MyTalkConfig *config) {
    struct pollfd fds[2];
    char line[256];
    int n;

    /* Check if windowing */
    if (!config->no_windowing) {
        start_windowing();
    }

    /* Setup pollfd settings */
    fds[0].fd = config->client_socket;
    fds[0].events = POLLIN;
    fds[1].fd = fileno(stdin);
    fds[1].events = POLLIN;

    while (1) {
        /* Attempt to poll */
        if (poll(fds, 2, -1) < 0) {
            perror("Poll failed");
            break;
        }

        /* If message recieved event */
        if (fds[0].revents & POLLIN) {
            /* Grab message */
            n = recv(config->client_socket, line, sizeof(line) - 1, 0);
            if (n <= 0) {
                if (n < 0) perror("Error reading from socket");
                break;
            }
            /* Nul terminate */
            line[n] = '\0';
            /* Write to output */
            write_to_output(line, n);
        }

        /* If message typed event */
        if (fds[1].revents & POLLIN) {
            /* Update input buffer */
            update_input_buffer();
            /* Check if a whole line is ready to be sent */
            if (has_whole_line()) {
                /* If true, read input */
                n = read_from_input(line, sizeof(line) - 1);
                if (n > 0) {
                    /* Nul terminate */
                    line[n] = '\0';
                    /* Send message */
                    send(config->client_socket, line, n, 0);
                /* Else chat is over */
                } else if (has_hit_eof()) {
                    /* Break */
                    break;
                }
            }
        }
    }

    /* Stop windowing */
    if (!config->no_windowing) {
        stop_windowing();
    }

    /* Close connection */
    close(config->client_socket);
    if (config->hostname == NULL) close(config->server_socket);
}

/* Initialize signal handler */
void setup_signal_handler(void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

/* Stops windowing and exits */
void handle_sigint(int sig) {
    stop_windowing();
    exit(EXIT_SUCCESS);
}