#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"
#include "syntax.tab.h"

extern FILE *yyin;
void yyrestart(FILE *);

bool error;

int main(int argc, char** argv)
{
    if (argc <= 1) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if(!error) {
        // print_tree(root, 0);
        Program(root);
    }
}
