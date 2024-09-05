#include<stdio.h>



int main(int argc, char* argv[]) {


    return 1;
}


char *strrchr(const char *s, int c) {
    char *last = NULL;
    /* This NULL check is what
     I didn't put on the test */
    if (s == NULL) {
        return NULL;
    }

    /* Loop until reach end of string */
    while (*s != '\0') {
        /* If we find the char point to that location */
        if (*s == c) {
            last = s;
        }
        s++;
    }
    /* If we reach the end and the char is '\0' */
    if (c == '\0') {
        return s;
    }
    /* return pointer to char */
    return last;
}