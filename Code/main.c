#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"
#include "ir.h"
#include "syntax.tab.h"

extern FILE *yyin;
void yyrestart(FILE *);

bool error;

int main(int argc, char** argv)
{
    if (argc <= 2) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    FILE *fw = fopen(argv[2], "w");
    if (!fw)
    {
        perror(argv[2]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if(!error) {
        // print_tree(root, 0);
        Program(root);
        translate_Program(root);
        print_ir(ir_head, fw);
    }
}
