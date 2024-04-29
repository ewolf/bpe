
// Define a struct to hold both the array of strings and the integer
typedef struct {
    unsigned char **cues;
    unsigned int *text;
    unsigned int text_size;
    unsigned int cues_size;
} TextCues;


TextCues* bpe_encode(const char*, unsigned int, char );
