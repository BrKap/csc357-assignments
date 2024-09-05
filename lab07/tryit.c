#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    pid_t pid;
    int status, exit_status;
    

    if (argc < 2) {
        fprintf(stderr, "usage: tryit command\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid == -1) {
        perror("Fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(argv[1], &argv[1]);

        perror(argv[1]);
        exit(EXIT_FAILURE);
    } else {

        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                printf("Process %d succeeded.\n", pid);
            } else {
                printf("Process %d exited with an error value.\n", pid);
            }
            return exit_status;
        } else {
            printf("Process %d did not exit normally.\n", pid);
        exit(EXIT_FAILURE);
        }
    }

    return 0;
}