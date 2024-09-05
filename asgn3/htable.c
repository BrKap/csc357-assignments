
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "htable.h"






/*
 * Reads chars from file and counts the frequency
 * Then stores in a histogram
 */
void build_histogram(int filename, unsigned int histogram[]) {
    char c;

    /* Reads char and increases count in histogram */
    while (read(filename, &c, 1) != 0) {
        /* fprintf(stderr, "char read = %c\n", c); */
        histogram[(uint8_t)c]++;
    }
}

/*
 * Function to help create nodes easier
 */
Node *create_node(uint8_t byte, int frequency) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    node->byte = byte;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/* 
 * Compares nodes by frequency least to greatest
 */
int compare_nodes(const void *a, const void *b) {
    const Node *node1 = *(const Node **)a;
    const Node *node2 = *(const Node **)b;
    return (node1->frequency - node2->frequency);
}

/*
 * Builds the huffman tree using frequencies
 * in the histogram
 */
Node *build_huffman_tree(unsigned int histogram[]) {

    Node *nodes[BYTE_SIZE];
    Node *left, *right, *parent;
    int num_nodes = 0;

    int i;
    /* 
     * Creates nodes for each char read in the file
     * and inputting its frequency as its data
     */
    for (i = 0; i < BYTE_SIZE; i++) {
        if (histogram[i] > 0) {
            nodes[num_nodes++] = create_node((uint8_t)i, histogram[i]);
        }
    }

    /* printf("Frequencies before sorting:\n");
    for (i = 0; i < num_nodes; i++) {
        printf("Node %d: frequency = %d\n", i, nodes[i]->frequency);
    } */

    /* sort the nodes by frequency */
    qsort(nodes, num_nodes, sizeof(Node *), compare_nodes);

    /* printf("Frequencies after sorting:\n");
    for (i = 0; i < num_nodes; i++) {
        printf("Node %d: frequency = %d\n", i, nodes[i]->frequency);
    } */

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

        /* printf("Frequencies before sorting:\n");
        for (i = 0; i < num_nodes; i++) {
            printf("Node %d: frequency = %d\n", i, nodes[i]->frequency);
        } */
        
        /* Resort the array by frequency again */
        qsort(nodes, num_nodes, sizeof(Node *), compare_nodes);

        /* printf("Frequencies after sorting:\n");
        for (i = 0; i < num_nodes; i++) {
            printf("Node %d: frequency = %d\n", i, nodes[i]->frequency);
        } */
    }

    return nodes[0];
}

/* 
 * Generate the encodings through recursion
 * If taking left path, add 0
 * If taking right path, add 1
 * Code ends when it hits a valid char
 * (No more left/right child)
 */
void generate_encodings(Node *root, char *code,
                        EncodingEntry encoding_table[]) {
    int len;
    char *left_code;
    char *right_code;
    if (root == NULL) return;

    /* Base case (valid char) */
    if (root->left == NULL && root->right == NULL) {
        /* fprintf(stderr, "code: %s\n", code); */
        /* Store the code at byte */
        encoding_table[root->byte].code = code;
    } else {
        /* Get len of code */
        len = strlen(code);
        /* allocate memory for right path and left path */
        left_code = (char *)malloc(len + 2); 
        right_code = (char *)malloc(len + 2);
        if (left_code == NULL || right_code == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        /* Copy code into both right and left */
        strcpy(left_code, code);
        strcpy(right_code, code);
        /* Appened code with 0 or 1 for left or right path */
        left_code[len] = '0';
        left_code[len + 1] = '\0';
        right_code[len] = '1';
        right_code[len + 1] = '\0';

        /* Recurse until base case */
        generate_encodings(root->left, left_code, encoding_table);
        generate_encodings(root->right, right_code, encoding_table);
    }
}


/*
 * Print out encodings
 */
void print_encoding_table(EncodingEntry encoding_table[]) {
    int i;
    for (i = 0; i < BYTE_SIZE; i++) {
        if (encoding_table[i].code != NULL) {
            /* 
             * print 0x and attach hex form of the char 
             * after it with at least 2 chars 
             */
            printf("0x%02x: %s\n", encoding_table[i].byte,
                                   encoding_table[i].code);
        }
    }
}
