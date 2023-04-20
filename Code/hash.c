#include "hash.h"
#include <stdlib.h>
#include <string.h>

unsigned int hash(const char *key, unsigned int prime) {
    unsigned long hash = 0;
    const char* p = key;
    while (*p)
        hash = (hash * prime) + (*p++);
    return hash % TABLE_SIZE;
}

HashTable* create_table() {
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    memset(ht->row_heads, 0, sizeof(ht->row_heads));
    memset(ht->col_heads, 0, sizeof(ht->col_heads));
    ht->prime = PRIME;
    return ht;
}

void insert(HashTable* ht, const char* key, void* value, int depth) {
    unsigned int row_index = hash(key, ht->prime);
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    new_node->key = key;
    new_node->value = value;
    new_node->prev = NULL;
    new_node->next = ht->row_heads[row_index];
    if (ht->row_heads[row_index])
        ht->row_heads[row_index]->prev = new_node;
    ht->row_heads[row_index] = new_node;
    new_node->down = ht->col_heads[depth];
    ht->col_heads[depth] = new_node;
}

void *find(HashTable* ht, const char *key) {
    unsigned int index = hash(key, ht->prime);
    HashNode* node = ht->row_heads[index];
    while (node) {
        if (!strcmp(node->key, key))
            return node->value;
        node = node->next;
    }
    return NULL;
}

void remove_column(HashTable* ht, int depth) {
    while (ht->col_heads[depth]) {
        HashNode* node = ht->col_heads[depth];
        unsigned int row_index = hash(node->key, ht->prime);
        if (ht->row_heads[row_index] == node) {
            ht->row_heads[row_index] = node->next;
            if (node->next)
                node->next->prev = NULL;
        } else {
            node->prev->next = node->next;
            if (node->next)
                node->next->prev = node->prev;
        }
        HashNode* next_node = node->down;
        ht->col_heads[depth] = next_node;
    }
}
