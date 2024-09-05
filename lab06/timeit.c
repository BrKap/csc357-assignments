#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "timeit.h"

/* Global variables so I don't
 * need to pass as parameters
 */
int remaining_time;
int is_tock = 0;

int main(int argc, char *argv[]) {
    char *endptr;
    struct sigaction sa;
    struct itimerval timer;
    int seconds;

    /* We only want 1 */
    if (argc != 2) {
        usage();
    }

    /* Convert arg string into an actual integer */
    seconds = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || seconds < 0) {
        fprintf(stderr, "%s: malformed time\n", argv[1]);
        usage();
    }

    /* Since we want to Tick and Tock multiply it by 2 */
    remaining_time = seconds * 2; 


    /* Initilize handler */
    sa.sa_handler = handle_sigalrm;
    /* Empty sig */
    sigemptyset(&sa.sa_mask);
    /* Clear flag */
    sa.sa_flags = 0;
    /* Setup sig */
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Set values for every 0.5 seconds */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;

    /* Set the intervals to repeat */
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }

    /* Repeat until done */
    while (1) {
        pause();
    }

    return 0;
}

/* Print Usage Message */
void usage(char *prog_name) {
    fprintf(stderr, "Usage: timeit <seconds>\n");
    exit(EXIT_FAILURE);
}

/* Handle alarm by Ticking or Tocking */
void handle_sigalrm(int sig) {
    /* If tock */
    if (is_tock) {
        /* Print Tock */
        printf("Tock\n");
        /* Flush stdout */
        fflush(stdout);
        /* Set tock to false */
        is_tock = 0;
    /* If tock is false */
    } else {
        /* Print Tick */
        printf("Tick...");
        /* Flush stdout */
        fflush(stdout);
        /* Set tock to true */
        is_tock = 1;
    }
    /* Decrease remaining time by 1 */
    remaining_time--;
    /* If time is up */
    if (remaining_time <= 0) {
        /* Print Time's up */
        printf("Time's up!\n");
        /* Exit Program as we are now done */
        exit(EXIT_SUCCESS);
    }
}
