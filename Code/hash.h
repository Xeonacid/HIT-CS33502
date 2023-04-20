#pragma once

#define TABLE_SIZE 65521
#define PRIME 2654435761

typedef struct HashNode {
    const char *key;
    void *value;
    struct HashNode *prev;
    struct HashNode *next;
    struct HashNode *down;
    int depth;
} HashNode;

typedef struct {
    HashNode *row_heads[TABLE_SIZE];
    HashNode *col_heads[TABLE_SIZE];
    unsigned int prime;
} HashTable;

HashTable* create_table();
void insert(HashTable* ht, const char *key, void *value, int depth);
void *find(HashTable* ht, const char *key);
void remove_column(HashTable* ht, int depth);
