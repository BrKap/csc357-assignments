


typedef struct Node {
    uint8_t byte;
    int frequency;
    struct Node *left;
    struct Node *right;
} Node;


typedef struct {
    Node *root;
} HuffmanTree;


typedef struct {
    uint8_t byte;
    char *code;
} EncodingEntry;


void build_histogram(const char *filename, unsigned int histogram[]);
int compare_nodes(const void *a, const void *b);
Node *build_huffman_tree(unsigned int histogram[]);
void generate_encodings(Node *root, char *code,
                        EncodingEntry encoding_table[]);
void print_encoding_table(EncodingEntry encoding_table[]);