#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_map.h"

hash_map* hash_map_init(){
    hash_map* hm = (hash_map*)malloc(sizeof(hash_map));
    if (hm == NULL)
        return NULL;

    hm->size = 0;
    hm->capacity = 100;
    hm->load_factor = 0.75;
    hm->multiplier = 2;

    hm->keys = (char**)malloc(hm->capacity * sizeof(char*));
    hm->values = (int*)malloc(hm->capacity * sizeof(int));
    hm->states = (char*)calloc(hm->capacity, sizeof(char));

    if (hm->states == NULL || hm->keys == NULL || hm->values == NULL){
        free(hm->keys);
        free(hm->values);
        free(hm);
        return NULL;
    }

    return hm;
}

unsigned int hash(char* key, int size) {
    unsigned int hash = 0;
    int c;
    while ((c = *key++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash % size;
}

int hash_map_put(hash_map *hm, char* key, int str_size, int value) {
    if ((hm->size + 1) > (hm->load_factor * hm->capacity)){
        if (hash_map_resize(hm))
            return 1;
    }

    unsigned int index = hash(key, hm->capacity);
    while (hm->states[index] == 2) {
        if (strcmp(hm->keys[index], key) == 0) {
            hm->values[index] = value;
            return 0;
        }
        index = (index + 1) % hm->capacity;
    }

    hm->keys[index] = (char*)malloc((str_size + 1) * sizeof(char));
    if (hm->keys[index] == NULL)
        return 1;

    strcpy(hm->keys[index], key);
    hm->states[index] = 2;
    hm->values[index] = value;
    hm->size++;
    return 0;
}

int hash_map_increment(hash_map* hm, char* key, int str_size){
    unsigned int index = hash(key, hm->capacity);
    while ((hm->states[index] == 2)) {
        if (strcmp(hm->keys[index], key) == 0) {
            hm->values[index]++;
            return 0;
        }
        index = (index + 1) % hm->capacity;
    }

    return hash_map_put(hm, key, str_size, 1);
}

int hash_map_get(hash_map* hm, char* key, int *value) {
    unsigned int index = hash(key, hm->capacity);
    while (hm->states[index] != 0) {
        if (strcmp(hm->keys[index], key) == 0) {
            *value = hm->values[index];
            return 0;
        }
        index = (index + 1) % hm->capacity;
    }
    return 0;
}

int hash_map_delete(hash_map* hm, char* key) {
    unsigned int index = hash(key, hm->capacity);
    while (hm->states[index] != 0) {
        if (hm->keys[index] != NULL && strcmp(hm->keys[index], key) == 0) {
            free(hm->keys[index]);
            hm->states[index] = 1;
            return 0;
        }
        index = (index + 1) % hm->capacity;
    }
    return 1;
}

int hash_map_resize(hash_map* hm){
    int old_capacity = hm->capacity;
    hm->capacity = hm->capacity * hm->multiplier;
    unsigned int new_index;

    char** new_keys = (char**)malloc(hm->capacity * sizeof(char*));
    int* new_values = (int*)malloc(hm->capacity * sizeof(int));
    char* new_states = (char*)calloc(hm->capacity, sizeof(char));
    
    if (new_keys == NULL || new_values == NULL || new_states == NULL){
        free(new_keys);
        free(new_states);
        free(new_values);
        return 1;
    }

    for (int i = 0; i < old_capacity; i++){
        if (hm->states[i] == 2){
            new_index = hash(hm->keys[i], hm->capacity);
            while (new_states[new_index] == 2)
    	        new_index = (new_index + 1) % hm->capacity;
                
            new_states[new_index] = 2;
            new_keys[new_index] = hm->keys[i];
            new_values[new_index] = hm->values[i];
            
        }
    }

    free(hm->keys);
    free(hm->values);
    free(hm->states);

    hm->keys = new_keys;
    hm->values = new_values;
    hm->states = new_states;

    return 0;
}

void hash_map_destroy(hash_map* hm){
    for (int i = 0; i < hm->capacity; i++) {
        if(hm->keys[i] != NULL && hm->states[i] == 2) {
            free(hm->keys[i]);
        }
    }
    free(hm->keys);
    free(hm->states);
    free(hm->values);
}


hash_map_iterator* create_iterator(hash_map* hash_map) {
    hash_map_iterator* it = (hash_map_iterator*)malloc(sizeof(hash_map_iterator));
    if (it == NULL)
        return NULL;
    it->hash_map = hash_map;
    it->currentIndex = -1;
    return it;
}


int next(hash_map_iterator* iterator, char **key, int *value) {
    hash_map *hm = iterator->hash_map;
    int capacity = hm->capacity;

    for (int i = iterator->currentIndex + 1; i < capacity; i++) {
        if (hm->states[i] == 2) {
            iterator->currentIndex = i;
            *key = hm->keys[i];
            *value = hm->values[i];
            return 1;
        }
    }

    return 0;
}

void iterator_destroy(hash_map_iterator* it) {
    free(it);
}
