#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "htable.h"
#include "hdecode.h"
#define USAGE "Usage: hdecode [ ( infile | - ) [ outfile ] ]"

int main(int argc, char* argv[]) {
    Node *root;
    int infile, outfile;
    
    /* If no args given, use stdin and stdout */
    if (argc == 1) {
        infile = STDIN_FILENO;
        outfile = STDOUT_FILENO;
    /* If 1 arg is given check if its a - for stdin
       or try to open the file given. Use stdout */
    } else if (argc == 2) {
        if (!strcmp(argv[1], "-")) {
            infile = STDIN_FILENO;
        } else {
            if (-1 == (infile = open(argv[1], O_RDONLY))) {
                perror("Error opening infile");
                exit(EXIT_FAILURE);
            }   
        }
        outfile = STDOUT_FILENO;
    /* If 2 args given, check once again for - or file given
       Then open 2nd arg for outfile */
    } else if (argc == 3) {
        if (!strcmp(argv[1], "-")) {
            infile = STDIN_FILENO;
        } else {
            if (-1 == (infile = open(argv[1], O_RDONLY))) {
                perror("Error opening infile");
                exit(EXIT_FAILURE);
            }   
            if (-1 == (outfile = open(argv[2],
                    O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))) {
                perror("Error opening outfile");
                exit(EXIT_FAILURE);
            }  
        } 
    } else {
        /* if more than 2 args are given, print usage message */
        printf(USAGE);
        exit(EXIT_FAILURE);
    }

    /* Create huffman tree */
    root = grab_hufftree(infile);

    /* if root is NULL, file is empty */
    if (root == NULL) {
        close(infile);
        close(outfile);
        return 1;
    }

    /* Decode the bit stream */
    decode_stream(infile, outfile, root);

    close(infile);
    close(outfile);
    return 1;
}

/*
 * Reads the header and creates the
 * huffman tree off of the chars and frequency
 */
Node *grab_hufftree(int filename) {
    int i, count;
    char c;
    uint8_t byte;
    Node *nodes[BYTE_SIZE], *left, *right, *parent;
    int num_nodes;
    char a[4];


    /* Checks if file is empty */
    if (read(filename, &c, 1) == -1) {
        return NULL;
    }
    /* sets the count to the number or unique chars */
    num_nodes = (uint8_t)c + 1;
    for (i = 0; i < num_nodes; i++) {
        /* For each char, get the byte and frequency */
        read(filename, &byte, 1);
        read(filename, &a, 4);

        /* Create node */
        count = htonl(*(int *)a);
        nodes[i] = create_node(byte, count);
    }

    qsort(nodes, num_nodes, sizeof(Node *), compare_nodes);

    /* 
     * Following the procedure in asgn3
     * Build huffman tree by taking first and 2nd Nodes
     * Creating a new node, setting left child as first node
     * and right child as right child, and the parent having
     * frequency of the sum
     */
    while (num_nodes > 1) {

        left = nodes[0];
        right = nodes[1];

        parent = create_node(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;

        /* Reinsert parent at index 0 */
        nodes[0] = parent;
        /* Move all nodes down 1 index */
        for (i = 1; i < num_nodes - 1; i++) {
            nodes[i] = nodes[i + 1];
        }
        num_nodes--;
        
        /* Resort the array by frequency again */
        qsort(nodes, num_nodes, sizeof(Node *), compare_nodes);

    }

    return nodes[0];

}

/*
 * Decodes the binary bits by traversing
 * the huffman tree
 */
void decode_stream(int infile, int outfile, Node *root) {
Node *current;

    int off = 7;
    int i, bit;
    char byte;

    /* If there is only 1 char, it means no bit stream */
    if (root->left == NULL && root->right == NULL) {
        /* Keep writing the char for the amount of frequency */
        while(root->frequency > 0) {
            write(outfile, &(root->byte), 1);
            root->frequency--;
        }
        return;
    }
    current = root;
    /* for each byte read, start traversing the tree */
    while (read(infile, &byte, 1) != -1) {
        for (i = 0; i < 8; i++) {
            bit = (byte >> off) & 1;
            
            /* if bit is 0, travel left */
            if (bit == 0) {
                current->frequency--;
                current = current->left;
            /* Else travel right */
            } else {
                current->frequency--;
                current = current->right;
            }
            /* If at leaf node*/
            if (current->left == NULL && current->right == NULL) {
                /* If root frequency is 0, stop writing */
                if (root->frequency < 0) {
                    return;
                }
                /* Write the byte at the leaf node */
                write(outfile, &(current->byte), 1);
                /* Reduce the frequency by 1 */
                current->frequency--;
                /* Reset current back to top of tree */
                current = root;
            }
            
            off--;
            /* If byte is fully read, reset it and read next char */
            if (off < 0) {
                off = 7;
            }
        }
    }
}