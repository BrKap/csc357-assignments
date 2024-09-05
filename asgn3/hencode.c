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
#include "hencode.h"
#define USAGE "Usage: hencode infile [ outfile ]"


int main(int argc, char* argv[]) {
    /* Initializes histogram to find the count of each char */
    unsigned int histogram[BYTE_SIZE] = {0};
    Node *root;
    int i;
    int infile, outfile;
    /* Creates an empty string to use as our base code for encoding */
    char code[BYTE_SIZE] = "";
    /* Allocates memory for the encoding table */
    EncodingEntry *encoding_table = (EncodingEntry *)malloc(
                                BYTE_SIZE * sizeof(EncodingEntry));
    if (argc <= 1) {
        printf(USAGE);
        exit(EXIT_FAILURE);
    }
    else if (argc <= 3) {
        if (argc == 3) {

            if (-1 == (outfile = open(argv[2],
                    O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))) {
                perror("Error opening outfile");
                exit(EXIT_FAILURE);
            }
        } else {
            outfile = STDOUT_FILENO;
        }

        if (-1 == (infile = open(argv[1], O_RDONLY))) {
            perror("Error opening infile");
            exit(EXIT_FAILURE);
        }
    } else {
        printf(USAGE);
        exit(EXIT_FAILURE);
    }

    /* Build the histogram */
    build_histogram(infile, histogram);

    /* Creates the huffman tree out of the histogram */
    root = build_huffman_tree(histogram);

    /* Initializes the encoding table */
    for (i = 0; i < BYTE_SIZE; i++) {
        encoding_table[i].byte = (uint8_t)i;
        encoding_table[i].code = NULL; 
    }

    /* Create the encoding for each char */
    generate_encodings(root, code, encoding_table);

    write_header(infile, outfile, histogram, encoding_table);

    close(infile);
    close(outfile);
    return 0;
}

void write_header(int fileinput, int fileoutput,
             unsigned int histogram[], EncodingEntry encoding_table[]) {
    
    uint8_t count = 0;
    int ncount = 0;
    uint8_t b = 0;
    int off, i;
    char c;
    Node *nodes[BYTE_SIZE]; 
    uint32_t frequency;

    for (i = 0; i < BYTE_SIZE; i++) {
        if (histogram[i] > 0) {
            nodes[count++] = create_node((uint8_t)i, histogram[i]);
        } else {
            ncount++;
        }
    }
    if (ncount == 256) {
        exit(EXIT_SUCCESS);
    }
    count--;
    write(fileoutput, &count, 1);

    for (i = 0; i <= count; i++) {
        write(fileoutput, &(nodes[i]->byte), 1 );

        frequency = htonl(nodes[i]->frequency);
        write(fileoutput, &frequency, 4);
    }



    if ((lseek(fileinput, 0, SEEK_SET)) == -1) {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    /* Bit Stream */
    b = 0;
    off = 7;
    while (read(fileinput, &c, 1) != 0) {
        for (i = 0; encoding_table[(uint8_t)c].code[i]; i++) {
            if (off == -1) {
                write(fileoutput, &b, 1);
                off = 7;
                b = 0;
            }
            if (encoding_table[(uint8_t)c].code[i] == '1') {
                b = b | 1 << off;
            }
            off--;
        }
    }
    if (off != 7) {
        write(fileoutput, &b, 1);
    }

}