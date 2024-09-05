#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "mypwd.h"

#define PATH_MAX 4096



int main(int argc, char *argv[]) {

    DIR *directory;
    struct dirent *element;
    struct stat fileinfo;
    ino_t current_inode, parent_inode, node;
    dev_t current_dev, parent_dev, dev;
    Node *path = NULL, *temp, *current;
    int is_linked = 1;

    /* Try opening current directory */
    if ((directory = opendir(".")) == NULL ) {
        perror("mypwd");
        exit(EXIT_FAILURE);
    }
    /* Read first element */
    element = readdir(directory);

    /* Get information about the current directory */
    if (lstat(element->d_name, &fileinfo) == 0) {
        /* Grab inode value */
        dev = fileinfo.st_dev;
        node = fileinfo.st_ino;
        /*puts(element->d_name); 
        printf("%ld\n", fileinfo.st_ino);*/
    }

    /*printf("Finished reading current directory\n");*/

    /* Continue looping until root is found */
    while (1) {
        if (!is_linked) {
            printf("Cannot get current directory\n");
            exit(EXIT_FAILURE);
        }
        /* Set is_linked to false */
        is_linked = 0;

        /* Change directory to parent */
        if ((chdir("..") == -1)) {
            perror("mypwd");
            exit(EXIT_FAILURE);
        }
        /* Open current directory (Which was parent before) */
        if ((directory = opendir(".")) == NULL ) {
            perror("mypwd");
            exit(EXIT_FAILURE);
        }

        /* Read current directory element */
        element = readdir(directory);
        /*printf("\nReading First Directory\n");
        printf("Directory name = %s\n", element->d_name);*/
        /* Grab information */
        if (lstat(element->d_name, &fileinfo) == 0) {
            /* Grab inode value */
            parent_dev = fileinfo.st_dev;
            parent_inode = fileinfo.st_ino;
        }

        /* Check if current is equal to parent (check if root) */
        if (current_inode == parent_inode && current_dev == parent_dev) {
            /*printf("Found root\n");*/
            /* Break from while loop */
            break;
        }

        /* After checking, update current */
        current_inode = parent_inode;
        current_dev = parent_dev;
        /* Rewind reading directory */
        rewinddir(directory);

        /* Loop through all elements in current directory */
        while ((element = readdir(directory))) {
            /* If for some reason we cannot stat element */
            if (lstat(element->d_name, &fileinfo) == -1) {
                /* Skip element */
                continue;
            }
            /*printf("Reading this directory: ");
            puts(element->d_name);     
            printf("%ld\n", fileinfo.st_ino); 
            printf("%ld\n", node);*/

            /* If we find the correct link */
            if (node == fileinfo.st_ino && dev == fileinfo.st_dev) {
                /*printf("\nFound Correct Directory!\n");
                printf("%s\n\n",element->d_name);*/

                /* Create a temp node */
                temp = (Node *)malloc(sizeof(Node *));
                /* Duplicate d_name to the node name */
                temp->name = strdup(element->d_name);
                /* Append a null character at end of string
                   There were weird problems regarding strlen()
                   occuring if I didn't do this */
                temp->name[(int)strlen(temp->name)] = '\0';
                /* Update node to be the current directory */
                node = current_inode;
                dev = current_dev;

                /*printf("Name = %s\n", temp->name);
                printf("Name size = %d\n", (int)strlen(temp->name));*/

                /* Checking if it's the first Node created */
                if (path == NULL) {
                    /*printf("Path was NULL\n");*/
                    /* Get strlen of directory name and add 1 for the '/' */
                    temp->size = strlen(temp->name) + 1;
                    temp->next = NULL;
                    path = temp;
                /* Do the same as above except temp->next
                 is now path instead of NULL representing the tail*/
                } else {
                    /*printf("Path was not NULL\n");*/
                    temp->size = strlen(temp->name) + path->size + 1;
                    temp->next = path;
                    path = temp;
                }
                /*printf("Path size = %d\n\n", path->size);*/
                /* If path size > 4096 */
                if (path->size > PATH_MAX) {
                    printf("Path too long\n");
                    exit(EXIT_FAILURE);
                }
                /* Set flag of linked to true and break */
                is_linked = 1;
                break;
            }
                           
        }
        /*printf("Finished Reading the previous directory\n");*/
    }
    /*printf("Printing out path\n");*/
    current = path;

    /* Iterate through linked list to print '/' and directory name */
    while (current != NULL) {
        if (!strcmp(current->name, ".")) {
            printf("/");  
            break;       
        }
        printf("/");
        printf("%s", current->name);
        current = current->next;
    }
    printf("\n");


    return 0;
}


