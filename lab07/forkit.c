#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>



int main(int argc, char* argv[]) {
    pid_t child;
    int status, inParent;

    if (argc != 1) {
        fprintf(stdout, "No arguments necessary\n");
        exit(EXIT_FAILURE);
    }

    inParent = (child = fork());

    if (child == -1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    
    if (!inParent) {
        child = getpid();
        fprintf(stdout, "This is child, pid %d\n", child);
    } else {
        child = getpid();
        fprintf(stdout, "This is parent, pid %d\n", child);

        if (-1 == wait(&status)) {
            perror("wait");
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "This is parent, pid %d\n", child);
    }

    return 0;
}