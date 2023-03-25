#include "node.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

node *root;

node* new_token_node(char* type, int lineno, char *val) {
    node *n = (node*)malloc(sizeof(node));
    n->type = type;
    n->lineno = lineno;
    n->is_token = true;
    n->child = NULL;
    n->sibling = NULL;
    if(!strcmp(type, "ID") || !strcmp(type, "TYPE")) {
        n->sval = (char*)malloc(strlen(val) + 1);
        strcpy(n->sval, val);
    }
    else if(!strcmp(type, "INT"))
        n->ival = strtol(val, NULL, 0);
    else if(!strcmp(type, "FLOAT"))
        n->fval = atof(val);
    return n;
}

node* new_non_terminal_node(char* type, size_t num, ...) {
    node *n = (node*)malloc(sizeof(node));
    n->type = type;
    n->is_token = false;
    n->child = NULL;
    n->sibling = NULL;
    if(num == 0) return n;

    va_list ap;
    va_start(ap, num);
    node *tmp = va_arg(ap, node*);
    n->child = tmp;
    n->lineno = tmp->lineno;
    for (int i = 1; i < num; ++i)
        tmp = tmp->sibling = va_arg(ap, node*);
    va_end(ap);
    return n;
}

void print_tree(node *n, size_t level) {
    if(!n || (!n->is_token && !n->child && !n->sibling))
        return;
    
    if(n->is_token || n->child) {
        for (int i = 0; i < level; ++i)
            printf("  ");
        printf("%s", n->type);
        if(!n->is_token)
            printf(" (%d)", n->lineno);
        else if(!strcmp(n->type, "ID") || !strcmp(n->type, "TYPE"))
            printf(": %s", n->sval);
        else if(!strcmp(n->type, "INT"))
            printf(": %d", n->ival);
        else if(!strcmp(n->type, "FLOAT"))
            printf(": %f", n->fval);
        puts("");
    }
    
    print_tree(n->child, level + 1);
    print_tree(n->sibling, level);
}
