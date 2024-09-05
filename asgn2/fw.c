#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "fw.h"
#define USAGE "usage: fw [-n num] [ file1 [ file 2 ...] ]\n"

/*
 * Takes argument of N top words to display
 * and files to read. Stores each word, and
 * then sorts them by wordcount, and displays
 * them
 */

int main(int argc, char *argv[]) {
	int fc, wordcount, i, n;
	FILE *currentf;
	char *word;
	HashTable *ht;
	Node **wordarr, *current;
/*
	if (argc == 1) {
		usageExit();	
	}
*/

/* initilizes default of 10 words and file index if no -n arg is provided */
	n = 10;
	fc = 1;

	
/* Checking invalid usage of -n */
	if (argc == 2 && strcmp(argv[1], "-n") == 0) {
		usageExit();
	}

/* Checking valid use of -n */

	if (argc > 2 && strcmp(argv[1], "-n") == 0) {
		n = atoi(argv[2]);
		fc = 3;
	}	
	
/* If n is negative it is not valid*/
	if (n <= 0) {
		usageExit();		
	}
/* Initialize hash table */
	ht = (HashTable *)malloc(sizeof(HashTable)); 	
	if (ht == NULL) {
		perror("ht alloc");
		exit(EXIT_FAILURE);		
	}

/* Clear the memory of the Hash Table */
	initHashTable(ht);


/* Iterate through the files if provided*/
	if (fc < argc) {

		for (i = fc; i < argc; i++) {
			currentf = fopen(argv[i], "r");
			if (currentf == NULL) {
		fprintf(stderr, "%s: No such file or directory\n", argv[i]);
				continue;
			}
		/* Read a word, then insert in Hash Table, repeat until done*/
			while ((word = readWord(currentf)) != NULL) {
				hashInsert(ht, word);
			}
					
			fclose(currentf);	
		}
	} else { /* Else read from stdin and do the same as before */
		while ((word = readWord(stdin)) != NULL) {
			if (word == NULL) {
			fprintf(stderr, "Trying to insert NULL into ht\n");
				exit(EXIT_FAILURE);
			}
			
			hashInsert(ht, word);
		}
	}

	
	wordcount = 0;
	/* Find the wordcount traversing the Hash Table*/
	for (i = 0; i < TABLE_SIZE; i++) {
		current = ht->collision[i];
		while (current != NULL) {
			wordcount++;
			current = current->next;
		}
	}
/* Create array of wordcount size */
	wordarr = (Node **)malloc(wordcount * sizeof(Node *));
	if (wordarr == NULL) {
		perror("wordarr allocatin");
		exit(EXIT_FAILURE);
	}
/* Reset wordcount */
	wordcount = 0;

/* Traverse Hash Table and put each word into array */
	for (i = 0; i < TABLE_SIZE; i++) {
		current = ht->collision[i];
		while (current != NULL) {
			wordarr[wordcount] = current;

			if (wordarr[wordcount]->word == NULL) {
				fprintf(stderr, "word was NULL\n");
				exit(EXIT_FAILURE);
			}
			wordcount++;		
			current = current->next;
		}	
	}	
	
/* old code for testing purposes
	for (i = 0; i < wordcount; i++) {
		if (wordarr[i] == NULL) {
			fprintf(stderr, "Node in wordarr was NULL\n");
		}
		word = wordarr[i]->word;
		fc = wordarr[i]->count;
		if (word == NULL) {
			fprintf(stderr, "word was NULL\n");
		}
	}
*/

/* Sort words by count */
	qsort(wordarr, wordcount, sizeof(Node *), compare);

/* Output the top words by count */
	printf("The top %d words (out of %d) are:\n", n, wordcount);	
	for (i = 0; i < wordcount && i < n; i++) {
		printf("%9d %s\n", wordarr[i]->count, wordarr[i]->word);
	}

/* Free memory */
	for (i = 0; i < wordcount; i++) {
		free(wordarr[i]->word);
		free(wordarr[i]);
	}
 	free(ht);	
	return 0;
}

/*
 * Function to compare Word Counts
 */

int compare(const void *a, const void *b) {
	const Node *node1 = *(const Node **)a;
	const Node *node2 = *(const Node **)b;

	if (node1->count != node2->count) {
		return node2->count - node1->count;
	}

	return strcmp(node2->word, node1->word);
}

/*
 * Function to print usage message and exit
 */

void usageExit() {
	fprintf(stderr, USAGE);
	exit(EXIT_FAILURE); 
}

/*
 * Hopefully creates a unique integer for the string
 * Otherwise later another function checks for collision
 * I did research this and I am using Daniel J. Berstein's
 * algorithm for computing this key for Strings
 * This is supposedly the one that does the best for Strings
 * so I am giving it a shot
 */
unsigned int hash(char *str) {
	unsigned int h = 5381;
	int c;
	/* Iterate through word and multiply the
     * h value by 33 and then add the character value
     */
	while ((c = *str++)) {
		h = ((h << 5) + h) + c;
	}

	/* Modulus to make the returned index value
 	 * valid for the Table Size
 	 */	

	h = h % TABLE_SIZE;
	return h;
}

/*
 * Function to set every value in the Hash Table
 * to NULL so it is convienent for later
 */

void initHashTable(HashTable *ht) {
	int i;
	for (i = 0; i < TABLE_SIZE; i++) {
		ht->collision[i] = NULL;
	}
}

/*
 * Function to insert the word into Hash Table
 * Also accounts for collision
 */

void hashInsert(HashTable *ht, char *word) {
	/* Gets index value for word */
	unsigned int index = hash(word);
	Node *newNode;
	Node *current;
/*	fprintf(stderr, "index = %d\n", index);*/

	/* Grabs the value at that index */
	current = ht->collision[index];
	
	/* These should never be true, but just in case*/
	if (index > TABLE_SIZE) {
		fprintf(stderr, "index is above Table Size\n");
	}
	if (word == NULL) {
		perror("word was NULL");
		usageExit();
	}
	
	/* Check if word already exists if it exists increase count by 1*/

	while (current != NULL) { 
		if (strcmp(current->word, word) == 0) {
			current->count++;
			return;
		} 
		current = current->next;
	}
	
	/* Else create new node*/

	newNode = (Node *)malloc(sizeof(Node));
	if (newNode == NULL) {
		perror("Node allocation\n");
		exit(EXIT_FAILURE);
	}
	/* Assign all the correct values to the node*/
	newNode->word = word;	
	newNode->count = 1;
	
	/* Put Node into linked list at that index*/

	newNode->next = ht->collision[index];
	ht->collision[index] = newNode;
}

/* Function I don't need anymore
int getCount(HashTable *ht, char *word) {
	unsigned int index = hash(word);
	Node *current = ht->collision[index];

	while (current != NULL) {
		if (strcmp(current->word, word) == 0) {
			return current->count;
		}
		current = current->next;
	}

	return 0;
}
*/

/*
 * Function to Read the next word
 */

char *readWord(FILE *input) {
	char *word, *temp;
	int size, len, c;
	
	size = 8;
	len = 0;
	word = (char *)malloc(size * sizeof(char));
	if (word == NULL) {
		perror("word allocation");
		exit(EXIT_FAILURE);
	}
	
	/*
 	 * Keep grabbing Characters until it reaches a non \n \t or space
 	 */

	while ((c = fgetc(input)) != EOF &&	!isalpha(c)) { 
			
	}
	
/*
 * If EOF is found, return NULL
 */

	if (c == EOF) {
		free(word);
		return NULL;
	}

/*
 * Since we already found a char
 * Always check if there is enough space
 * Then check if alpha and set to lower
 * Then append the char to the word
 * and lastly grab the next character
 * Then keep repeating until character
 * is EOF, \n, \t, or a space 
 */
	do {
		if (len + 1 >= size) {
			size += 8;
			temp = (char *)realloc(word, size + 1);
			if (temp == NULL) {
				perror("word realloc");
				free(word);
				exit(EXIT_FAILURE);
			}
			word = temp;
		}

		/* If c is a-z lowercase it */				
		if (isalpha(c)) {
			c = tolower(c);
		}

		/* Append char to end of word */
		word[len++] = (char)c;
	
		/* Grab next char */
		c = fgetc(input);
		
		
	} while(c != EOF && isalpha(c)); 

	/*
 	 * Append \0 to end to signify end of string
 	 */
	word[len] = '\0';

	if (len == 0 && c == EOF) {
		free(word);
		return NULL;
	}
/* Realloc word size to be the len + 1 so we don't waste space */
	if (size > len) {
		temp = (char *)realloc(word, len + 1);
		if (temp == NULL) {
			perror("word realloc");
			free(word);
			exit(EXIT_FAILURE);
		}
		word = temp;
	}
	
	return word;
}






