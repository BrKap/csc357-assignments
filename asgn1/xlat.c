/*
 * CSC 357, Assignment 1
 * Program to run xlat. 
 * Contains translate, destroy, and complement functionality
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xlat.h"
#define USAGE "usage: xlat [ -d ] [ -c ] set1 [ set2 ]\n"

/*
 * Grabs command line args, deciphers it,
 * and selects either translate, delete, or complement.
 * Prints Usage message if error occurs.
 */

int main(int argc, char *argv[]) {
    /*if the amount of arguments are too little or too big*/
    if (argc < 3 || argc > 4) {
	usageExit(); /*print usage message and stop program*/
    } else if (argc == 3) { /*if 3 args, either its delete or translate*/
        if (!strcmp(argv[1], "-d")) { /*if index 1 is "-d" its delete mode*/
	    /*run delete()*/
	    delete(argv[2]); /*run delete mode using index 2 as set1*/
	    return 0;
	} else {
	    /*assume argv[1] is set 1 and argv[2] is set 2*/
	    translate(argv[1], argv[2]);
	    return 0;
	}
    } else if (argc == 4) { /*if 4 args then it can only be complement*/
	if (!strcmp(argv[1], "-d") && !strcmp(argv[2], "-c")) {
	    /*run complement()*/
	    complement(argv[3]);
	    return 0;
	
	} else {
	    /*assume error as complement mode wasn't satisfied*/
	    usageExit();
	}
    }
    usageExit();
    /*It should never reach here, but just in case send usage message*/
    return 0;
}
/*
 * Takes set1 and set2 as inputs
 * If standard input is in set 1, translate to set 2
 * then put in standard output
 */
void translate(char *set1, char *set2) {
    int translation[256]; /*initialize a set of 256 to accomadate all chars*/
    int c, i, len1, len2, temp; /*initialize variables needed*/
    len1 = strlen(set1); /*find lengths of both set 1 and set 2*/
    len2 = strlen(set2);
    
    if (len2 == 0) { /*if set2 is empty, then send Usage message*/
	usageExit();
    }
     
    for (i = 0; i < 256; i++) { /*There might be a better way to do this*/
        translation[i] = -1;
        /*set initialize array to all be -1 just in case junk data exists*/
    }
     
    if (len1 > 0) { /*check if set1 is not empty*/   
        for (i = 0; i < len1; i++) {
            if (set1[i] > 255 || set1[i] < 0) {
	        /*if value is not a valid char then exit*/
                continue;
             }
	    /*set up for loop for all values of set1*/
            if (i > len2 - 1) {
	        /*this is to check if set2 has less values than set1*/
                translation[set1[i]] = temp; 
		/*basically expands the end of set2 
		 * to fill with	 last value of set2*/
            } else {
                temp = set2[i]; /*update temp, stops at last value of set2*/
                translation[set1[i]] = set2[i]; 
		/*put translated char value at index of value to translate*/
            }
        }
    }
    while ((c = getchar()) != EOF) { /*keep grabbing next char until EOF*/
        if (c > 255 || c < 0) { /*if not a valid char then exit*/
           continue;
        } else { 
	    /* check translation array at index of char is not -1
 	     * basically if it has a value to translate to*/
            putchar(translation[c] != -1 ? translation[c] : c); 
	    /*if not -1, grab translate the char, else don't translate*/
	}
    }
}

/*
 * Runs the Delete funtionality of xlat
 * Takes in a single set of chars
 */
void delete(char *set1) {
    int c;
/*Keeps grabbing the next char until EOF*/
    while ((c = getchar()) != EOF) {
	/* If c is 0 aka NULL putchar,
	 * for some reason the other if
	 * statement was not handling it
	 * */
	if (c == 0) {
	    putchar(c);
	    continue;
	} 
	/*
	 * If c is not in set1 putchar
	 */
        if (strchr(set1, c) == NULL) {
	    putchar(c);
        }
    }    
}   

/*
 * Runs the Complement functionality of xlat
 * Takes in a single set of chars
 */
void complement(char *set1) {
    int c, len1;
    /* Get len of set1 */
    len1 = strlen(set1);
    /* if set1 has values run code, 
     * otherwise don't run because no chars to keep */
    if (len1 > 0) {
	/* Keeps grabbing the next char until EOF */
        while ((c = getchar()) != EOF) {
	    /* If c is in set1 and is a valid char*/
            if (strchr(set1, c) != NULL && c < 256 && c > 0) {
	        putchar(c);
	    }
        }
    }
}

/*
 * Prints Usage Message and Exits the program
 */
void usageExit() {
    fprintf(stderr, USAGE);
    exit(EXIT_FAILURE);
}
