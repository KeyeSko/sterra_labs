#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "hash_map.h"

#define MAX_WORD_LEN 255

int get_word(FILE *fp, char *word) {
    int ch, i = 0;

    while ((ch = fgetc(fp)) != EOF && !isalpha(ch)) {
    }

    if (ch != EOF) {
        word[i++] = tolower(ch);
    }
    while (i < MAX_WORD_LEN - 1 && (ch = fgetc(fp)) != EOF && isalpha(ch)) {
        word[i++] = tolower(ch);
    }

    word[i] = '\0';
    
    return i;
}

int main(int argc, char *argv[]){
    
    hash_map *hm;
    hm = hash_map_init();
    if (hm == NULL)
        return 1;

    int size;
    char word[MAX_WORD_LEN] = {0};


    if (argc != 2){
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL){
        perror("main");
        return 1;
    }

    while ((size = get_word(fp, word)) != 0){
        hash_map_increment(hm, word, size);
    }

    fclose(fp);
    printf("Word frequency in the file.\n");
    
    hash_map_iterator *it;
    it = create_iterator(hm);
    if (it == NULL){
        hash_map_destroy(hm);
        return 1;
    }
    char *key;
    int value;

    while (next(it, &key, &value)){
        printf("Word: %s, Count: %d\n", key, value);
    }


    hash_map_destroy(hm);
    iterator_destroy(it);
    return 0;
}
