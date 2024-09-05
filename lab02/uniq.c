#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "uniq.h"

/*
 * Main file call, just runs uniq() with stdin
 */

int main(int argc, char *argv[]) {
    uniq(stdin);
    return 0;
}

/*
 * Takes FILE input and deletes consecutive duplicate lines
 */
void uniq(FILE *input) {
    char *prevLine = NULL;
    char *currLine;
     
    /*fprintf(stderr, "First Line Read\n");*/

/*
 * Reads the first line
 */
    currLine = read_long_line(input);

/*
 * If it is NULL, means there is no line
 */
    if (currLine == NULL) {
	return;
    }
    /*fprintf(stderr, "First Line Read Down\n");*/
    
/*
 * Print the currLine and move to prevLine
 */

    printf("%s\n", currLine);
    prevLine = currLine;

/*
 * Keep setting currLine to the next line until that function returns NULL
 */
    while ((currLine = read_long_line(input)) != NULL) {
		/*fprintf(stderr, "Testing if currLine is same as prevLine\n");*/
		/* Compare the currLine and the prevLine to see if they are the same*/
		if ((strcmp(currLine, prevLine)) != 0) {
			/* 
 			 * If they are not the same, print currLine 
 			 * Then free memory of prevLine
 			 * And lastly move currLine to new prevLine
 			 */
	    	/*fprintf(stderr, "Line was not the same\n");*/
	    	printf("%s\n", currLine);
	    	free(prevLine);
	    	prevLine = currLine;
		} else {
			/* If Line is the same, don't print, and free currLine */
	    	/*fprintf(stderr, "Line was the same\n");*/
	    	free(currLine);
		}
    }
	/*Once its done reading lines, Free prevLine*/
    /*fprintf(stderr, "Done Reading File\n");*/
    free(prevLine);
}


/*
 * This function has input of FILE
 * Reads until hits new line character or EOF
 * and puts results into a buffer and returns
 * a char pointer to the line read
 */
char *read_long_line(FILE *file) {
    char *line = NULL;
    size_t size = 0;
    size_t index = 0;
    int c;
    char *templine;

    /*fprintf(stderr , "running read_long_line\n");*/
	/* Initial memory for the line */
    line = (char *)malloc(size + 1);
    if (line == NULL) {
	/*fprintf(stderr, "malloc fail\n");*/
	return NULL;
    }

	/*
 	 * Set variable c to the next char read
 	 * Keep running until EOF is hit or 
 	 * new line character is hit
 	 */
    while ((c = fgetc(file)) != EOF && c != '\n') {
		/*fprintf(stderr, "grabbed character: %c\n", (char)c);*/
	
		/*
     	* Set the char read at the next point in the buffer
     	* the index post increments so its ready for the next
     	* char
     	*/
 
		line[index++] = (char)c;


		/* If the index exceeds buffer size */
		if (index >= size) {

			/* Increase size by 32 */
		    /*fprintf(stderr, "increasing size with realloc\n");*/
	    	size = size + 32;
			/* Realloc to a temp variable with new size */
	    	templine = (char *)realloc(line, size + 1);
	    	if (templine == NULL) {
				free(line);
				perror("realloc");
				/*fprintf(stderr, "realloc fail\n");*/
				return NULL;	
	    	}
			/* Set original buffer to point to the realloc */
	    	line = templine;    
		}	
    }
    
    /*fprintf(stderr, "finished grabbing line\n");*/
    
	/* if c is not new line and the index is 0
     * That means it hit EOF and is trying to run
     * the function again where there is no new line
     * so return NULL to finish uniq()
     */
    if (c != '\n' && index == 0) {
		free(line);
		/*fprintf(stderr, "Not new line and index = 0\n");*/
		return NULL;
    }
	
	/* If c = EOF of newline char then it
     * means its the end of a line so attach
     * NULL char to the end to mark end of str
     */
    if (c == EOF || c == '\n') {
		line[index] = '\0';
    }
    
    /*fprintf(stderr, "Returning line to currLine\n");*/
    
    return line;
}

