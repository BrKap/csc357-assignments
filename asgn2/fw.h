#define TABLE_SIZE 1000




typedef struct Node {
	char *word;
	int count;
	struct Node *next;
} Node;

typedef struct HashTable {
	Node *collision[TABLE_SIZE]; 
} HashTable;

unsigned int hash(char *str);
void initHashTable(HashTable *ht);
void hashInsert(HashTable *ht, char *word);
int compare(const void *a, const void *b);
char *readWord(FILE *input);
void usageExit();
