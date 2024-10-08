
typedef struct{
    char** keys;
    int* values;
    char* states; // 0 - NONE, 1 - Deleted, 2 - Busy
    float load_factor;
    int size;
    int capacity;
    float multiplier;
} hash_map;

unsigned int hash(char*, int);

hash_map* hash_map_init();

int hash_map_put(hash_map*, char*, int, int);

int hash_map_get(hash_map*, char*, int*);

int hash_map_resize(hash_map*);

int hash_map_delete(hash_map*, char*);

int hash_map_increment(hash_map*, char*, int);

void hash_map_destroy(hash_map*);

typedef struct hash_map_iterator{
    hash_map *hash_map;
    int currentIndex;
} hash_map_iterator;

hash_map_iterator* create_iterator(hash_map*);

int next(hash_map_iterator*, char**, int*);

void iterator_destroy(hash_map_iterator*);
