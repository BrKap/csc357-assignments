#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void execute_pipeline(char ***commands, int num_commands) {
    pid_t pid;
    int i, j;
    int **pipefds = malloc((num_commands - 1) * sizeof(int *));
    if (pipefds == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < num_commands - 1; i++) {
        pipefds[i] = malloc(2 * sizeof(int));
        if (pipefds[i] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }



    for (i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefds[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_commands; i++) {
        pid = fork();
        if (pid == 0) {
            if (i != 0) {
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            if (i != num_commands - 1) {
                if (dup2(pipefds[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            for (j = 0; j < num_commands - 1; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            if (execvp(commands[i][0], commands[i]) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_commands - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    for (i = 0; i < num_commands; i++) {
        wait(NULL);
    }

    for (i = 0; i < num_commands - 1; i++) {
        free(pipefds[i]);
    }
    free(pipefds);
}

int main(int argc, char *argv[]) {
    char *cmd1[] = {"ls", NULL};
    char *cmd2[] = {"tac", NULL};
    char **commands[] = {cmd1, cmd2};

    int num_commands = sizeof(commands) / sizeof(commands[0]);

    execute_pipeline(commands, num_commands);

    return 0;
}
