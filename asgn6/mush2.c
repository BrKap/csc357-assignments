#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mush.h>

#include "mush2.h"

#define PROMPT "8-P "

/* Global variable for interrupt flag */
int interrupted = 0;

int main(int argc, char *argv[]) {
    char *line;
    pipeline pipeline;
    FILE *input = stdin;

    /* Initilize shell */
    init_shell();

    /* Check if more than 1 arg */
    if (argc > 1) {
        printf("usage: mush2\n");
        exit(EXIT_FAILURE);
    }

    /* Shell loop */
    while (1) {
        /* If stdin is input, print prompt */
        print_prompt();

        /* Read next input line */
        line = readLongString(input);
        /* If error or EOF */
        if (!line) {
            /* Check if eof */
            if (feof(input)) {
                printf("\n");
                /* Stop loop */
                break;
            }
            /* Else check if error then just continue */
            if (ferror(input) && clerror == E_EMPTY) {
                clearerr(input);
                continue;
            } else {
                perror("readLongString");
            }
            continue;
        }


        /* Turn line read into pipeline */
        pipeline = crack_pipeline(line);
        if (!pipeline) {
            fprintf(stderr, "Parsing error\n");
            free(line);
            continue;
        }

        /* Debug purposes */
        /*print_pipeline(stdout, pipeline);*/


        /* If interupt before execution */
        if (interrupted) {
            /* Reset flag */
            interrupted = 0;
            /* Free memory */
            free_pipeline(pipe);
            free(line);
            /* restart loop */
            continue;
        }

        /* Check if cd */
        if (handle_cd(pipeline)) {
            free_pipeline(pipeline);
            free(line);
            continue;
        }

        /* Try to execute pipeline */
        execute_pipeline(pipeline);

        /* Free memory */
        free_pipeline(pipeline);
        free(line);
    }

    /* If input wasn't stdin, attempt to close */
    if (input != stdin) fclose(input);
    return 0;
}
/* Initilizes the shell */
void init_shell() {
    struct sigaction sa;
    /* Starts sig handler */
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

void handle_sigint(int sig) {
    interrupted = 1;
    printf("\n");
}

/* Prints prompt and flushes stdout */
void print_prompt() {
    if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
        printf(PROMPT);
        fflush(stdout);
    }

}

/* 
void setup_redirection(struct clstage *stage) {
    if (stage->inname) {
        int fd = open(stage->inname, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (stage->outname) {
        int fd = open(stage->outname,
                O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

void setup_pipes(int n, int **pipefds, int i) {
    int j;
    if (i > 0) {
        if (dup2(pipefds[i - 1][0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    if (i < n - 1) {
        if (dup2(pipefds[i][1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }

    for (j = 0; j < n - 1; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }

}
*/
/* Executes commands in pipeline */
void execute_pipeline(pipeline pipeline) {
    int status, i, j;
    int n = pipeline->length;
    /* Malloc number of commands - 1 */
    int **pipefds = malloc((n - 1) * sizeof(int *));
    pid_t pid;
    int infile, outfile;
    /*fprintf(stderr, "n = %d\n", n);*/

    /* For each pipe, create the read and write fd malloc */
    for (i = 0; i < n - 1; i++) {
        pipefds[i] = malloc(2 * sizeof(int));
        if (pipefds[i] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        /* Create the pipe */
        if (pipe(pipefds[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    /* For each pipe, fork and connect pipe properly, then exec */
    for (i = 0; i < n; i++) {
        pid = fork();
        /* If child */
        if (pid == 0) {
            /* If not first command */
            if (i != 0) {
                /* Dup2 pipfd of previous read into STDIN */
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            /* Else if first command and has valid inname*/
            } else if (pipeline->stage[i].inname != NULL) {
                /* Try to open redir input */
                infile = open(pipeline->stage[i].inname, O_RDONLY);
                if (infile == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                /* Dup redir into STDIN */
                if (dup2(infile, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                /* Close extra fd */
                close(infile);
            }

            /* If not last command */
            if (i != n - 1) {
                /* Dup2 previous pipe write to stdout */
                if (dup2(pipefds[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            /* Else if last command and valid outname */
            } else if (pipeline->stage[i].outname != NULL) {
                /* Attempt to open redir */
                outfile = open(pipeline->stage[i].outname,
                        O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (outfile == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                /* Redir new outfile to STDOUT */
                if (dup2(outfile, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                /* Close extra fd */
                close(outfile);
            }

            /* Close each pipe fd */
            for (j = 0; j < n - 1; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            /* Exec the command */
            /*fprintf(stderr, "%s\n", pipeline->stage[i].argv[0]);*/
            if (execvp(pipeline->stage[i].argv[0],
                    pipeline->stage[i].argv) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        /* Fork failed */
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    /* For parent, close all pipes that are unecessary */
    for (i = 0; i < n - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    /* Wait for each child */
    for (i = 0; i < n; i++) {
        wait(&status);
    }

    /* Free memory for each pipe fds */
    for (i = 0; i < n - 1; i++) {
        free(pipefds[i]);
    }
    /* Free the array of pipefds */
    free(pipefds);
}

/* Function to handle a cd */
int handle_cd(pipeline pipeline) {
    char *home;
    /* Check if first arg is "cd" return 0 if false */
    if (pipeline->length != 1 || strcmp(pipeline->stage[0].argv[0],
                "cd") != 0) return 0;

    /* If cd is only arg */
    if (pipeline->stage[0].argc == 1) {
        /* Get home */
        home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "unable to determine home directory\n");
            return 1;
        }
        /* Chdir to home */
        if (chdir(home) != 0) {
            perror("chdir");
            return 1;
        }
    /* Else if 2 args */
    } else if (pipeline->stage[0].argc == 2) {
        /* Chdir to the 2nd arg */
        if (chdir(pipeline->stage[0].argv[1]) != 0) {
            perror("chdir");
            return 1;
        }
    /* Else invalid arg count */
    } else {
        fprintf(stderr, "cd: too many arguments\n");
    }
    /* Return success */
    return 1;
}

