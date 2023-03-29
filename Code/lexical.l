%{
#include <stdio.h>
#include "node.h"
#include "syntax.tab.h"
int yycolumn = 1;
#define YY_USER_ACTION \
yylloc.first_line = yylloc.last_line = yylineno; \
yylloc.first_column = yycolumn; \
yylloc.last_column = yycolumn + yyleng - 1; \
yycolumn += yyleng;
%}

%option yylineno

INT 0|[1-9][0-9]*|0x[0-9a-fA-F]+|0[0-7]+
FLOAT [0-9]+\.[0-9]+([eE][+-]?[0-9]+)?
ID [_a-zA-Z][0-9_a-zA-Z]*
RELOP >|<|>=|<=|==|!=
TYPE int|float

EOL [\r]?\n
LINE_COMMENT \/\/.*{EOL}
%%
[ \t]
{EOL}|{LINE_COMMENT} { yycolumn = 1; }
"/*" {
    for(;;) {
        char c;
        while ((c = input()) != '*' && c);
        if (!c) {
            fprintf(stderr, "Error type A at Line %d: Unterminated comment.\n", yylineno);
            error = true;
            break;
        }
        if (c == '*') {
            c = input();
            if (c == '/')
                break;
        }
    }
}
{INT} { yylval.node = new_token_node("INT", yylineno, yytext); return INT; }
{FLOAT} { yylval.node = new_token_node("FLOAT", yylineno, yytext); return FLOAT; }
{TYPE} { yylval.node = new_token_node("TYPE", yylineno, yytext); return TYPE; }
";" { yylval.node = new_token_node("SEMI", yylineno, NULL); return SEMI; }
"," { yylval.node = new_token_node("COMMA", yylineno, NULL); return COMMA; }
"=" { yylval.node = new_token_node("ASSIGNOP", yylineno, NULL); return ASSIGNOP; }
{RELOP} { yylval.node = new_token_node("RELOP", yylineno, NULL); return RELOP; }
"+" { yylval.node = new_token_node("PLUS", yylineno, NULL); return PLUS; }
"-" { yylval.node = new_token_node("MINUS", yylineno, NULL); return MINUS; }
"*" { yylval.node = new_token_node("STAR", yylineno, NULL); return STAR; }
"/" { yylval.node = new_token_node("DIV", yylineno, NULL); return DIV; }
"&&" { yylval.node = new_token_node("AND", yylineno, NULL); return AND; }
"||" { yylval.node = new_token_node("OR", yylineno, NULL); return OR; }
"." { yylval.node = new_token_node("DOT", yylineno, NULL); return DOT; }
"!" { yylval.node = new_token_node("NOT", yylineno, NULL); return NOT; }
"(" { yylval.node = new_token_node("LP", yylineno, NULL); return LP; }
")" { yylval.node = new_token_node("RP", yylineno, NULL); return RP; }
"[" { yylval.node = new_token_node("LB", yylineno, NULL); return LB; }
"]" { yylval.node = new_token_node("RB", yylineno, NULL); return RB; }
"{" { yylval.node = new_token_node("LC", yylineno, NULL); return LC; }
"}" { yylval.node = new_token_node("RC", yylineno, NULL); return RC; }
"struct" { yylval.node = new_token_node("STRUCT", yylineno, NULL); return STRUCT; }
"return" { yylval.node = new_token_node("RETURN", yylineno, NULL); return RETURN; }
"if" { yylval.node = new_token_node("IF", yylineno, NULL); return IF; }
"else" { yylval.node = new_token_node("ELSE", yylineno, NULL); return ELSE; }
"while" { yylval.node = new_token_node("WHILE", yylineno, NULL); return WHILE; }
{ID} { yylval.node = new_token_node("ID", yylineno, yytext); return ID; }
. { fprintf(stderr, "Error type A at Line %d: Mysterious character \"%s\".\n", yylineno, yytext); error = true; }
