#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct node {
    char *type;
    int lineno;
    bool is_token;
    union {
        char *sval;
        int ival;
        double fval;
    };
    struct node *child;
    struct node *sibling;
} node;

extern node *root;
node *new_token_node(char *type, int lineno, char *val);
node *new_non_terminal_node(char *type, size_t num, ...);
void print_tree(node *n, size_t level);

extern bool error;
