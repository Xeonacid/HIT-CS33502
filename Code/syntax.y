%{
#include "node.h"
%}

%locations
%define parse.error verbose

%union{
    node *node; 
}
%token <node> INT
%token <node> FLOAT
%token <node> TYPE
%token <node> SEMI
%token <node> COMMA
%token <node> ASSIGNOP
%token <node> RELOP
%token <node> PLUS MINUS STAR DIV
%token <node> DOT
%token <node> AND OR
%token <node> NOT 
%token <node> LP RP LB RB LC RC
%token <node> STRUCT
%token <node> RETURN
%token <node> IF
%token <node> ELSE
%token <node> WHILE
%token <node> ID
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def Dec DecList
%type <node> Exp Args

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%right DOT LP RP LB RB
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%%
Program : ExtDefList { root = $$ = new_non_terminal_node("Program", 1, $1); }
        ;
ExtDefList : ExtDef ExtDefList { $$ = new_non_terminal_node("ExtDefList", 2, $1, $2); }
           | { $$ = new_non_terminal_node("ExtDefList", 0); }
           ;
ExtDef : Specifier ExtDecList SEMI { $$ = new_non_terminal_node("ExtDef", 3, $1, $2, $3); }
       | Specifier SEMI { $$ = new_non_terminal_node("ExtDef", 2, $1, $2); }
       | Specifier FunDec CompSt { $$ = new_non_terminal_node("ExtDef", 3, $1, $2, $3); }
       | Specifier FunDec SEMI { $$ = new_non_terminal_node("ExtDef", 3, $1, $2, $3); }
       ;
ExtDecList : VarDec { $$ = new_non_terminal_node("ExtDecList", 1, $1); }
           | VarDec COMMA ExtDecList { $$ = new_non_terminal_node("ExtDecList", 3, $1, $2, $3); }
           ;
Specifier : TYPE { $$ = new_non_terminal_node("Specifier", 1, $1); }
          | StructSpecifier { $$ = new_non_terminal_node("Specifier", 1, $1); }
          ;
StructSpecifier : STRUCT OptTag LC DefList RC { $$ = new_non_terminal_node("StructSpecifier", 5, $1, $2, $3, $4, $5); }
                | STRUCT Tag { $$ = new_non_terminal_node("StructSpecifier", 2, $1, $2); }
                ;
OptTag : ID { $$ = new_non_terminal_node("OptTag", 1, $1); }
       | { $$ = new_non_terminal_node("OptTag", 0); }
       ;
Tag : ID { $$ = new_non_terminal_node("Tag", 1, $1); }
    ;
VarDec : ID { $$ = new_non_terminal_node("VarDec", 1, $1); }
       | VarDec LB INT RB { $$ = new_non_terminal_node("VarDec", 4, $1, $2, $3, $4); }
       | error RB { error = true; }
       ;
FunDec : ID LP VarList RP { $$ = new_non_terminal_node("FunDec", 4, $1, $2, $3, $4); }
       | ID LP RP { $$ = new_non_terminal_node("FunDec", 3, $1, $2, $3); }
       | error RP { error = true; }
       ;
VarList : ParamDec COMMA VarList { $$ = new_non_terminal_node("VarList", 3, $1, $2, $3); }
        | ParamDec { $$ = new_non_terminal_node("VarList", 1, $1); }
        ;
ParamDec : Specifier VarDec { $$ = new_non_terminal_node("ParamDec", 2, $1, $2); }
         ;
CompSt : LC DefList StmtList RC { $$ = new_non_terminal_node("CompSt", 4, $1, $2, $3, $4); }
       | error RC { error = true; }
       ;
StmtList : Stmt StmtList { $$ = new_non_terminal_node("StmtList", 2, $1, $2); }
         | { $$ = new_non_terminal_node("StmtList", 0); }
         ;
Stmt : Exp SEMI { $$ = new_non_terminal_node("Stmt", 2, $1, $2); }
     | CompSt { $$ = new_non_terminal_node("Stmt", 1, $1); }
     | RETURN Exp SEMI { $$ = new_non_terminal_node("Stmt", 3, $1, $2, $3); }
     | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE { $$ = new_non_terminal_node("Stmt", 5, $1, $2, $3, $4, $5); }
     | IF LP Exp RP Stmt ELSE Stmt { $$ = new_non_terminal_node("Stmt", 7, $1, $2, $3, $4, $5, $6, $7); }
     | WHILE LP Exp RP Stmt { $$ = new_non_terminal_node("Stmt", 5, $1, $2, $3, $4, $5); }
     | error SEMI { error = true; }
     ;
DefList : Def DefList { $$ = new_non_terminal_node("DefList", 2, $1, $2); }
        | { $$ = new_non_terminal_node("DefList", 0); }
        ;
Def : Specifier DecList SEMI { $$ = new_non_terminal_node("Def", 3, $1, $2, $3); }
    | Specifier error SEMI { error = true; }
    ;
DecList : Dec { $$ = new_non_terminal_node("DecList", 1, $1);}
        | Dec COMMA DecList { $$ = new_non_terminal_node("DecList", 3, $1, $2, $3); }
        ;
Dec : VarDec { $$ = new_non_terminal_node("Dec", 1, $1); }
    | VarDec ASSIGNOP Exp { $$ = new_non_terminal_node("Dec", 3, $1, $2, $3); }
    ;
Exp : Exp ASSIGNOP Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp AND Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp OR Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp RELOP Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp PLUS Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp MINUS Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp STAR Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp DIV Exp { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | LP Exp RP { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | MINUS Exp { $$ = new_non_terminal_node("Exp", 2, $1, $2); }
    | NOT Exp { $$ = new_non_terminal_node("Exp", 2, $1, $2); }
    | ID LP Args RP { $$ = new_non_terminal_node("Exp", 4, $1, $2, $3, $4); }
    | ID LP RP { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | Exp LB Exp RB { $$ = new_non_terminal_node("Exp", 4, $1, $2, $3, $4); }
    | Exp DOT ID { $$ = new_non_terminal_node("Exp", 3, $1, $2, $3); }
    | ID { $$ = new_non_terminal_node("Exp", 1, $1); }
    | INT { $$ = new_non_terminal_node("Exp", 1, $1); }
    | FLOAT { $$ = new_non_terminal_node("Exp", 1, $1); }
    ;
Args : Exp COMMA Args { $$ = new_non_terminal_node("Args", 3, $1, $2, $3); }
     | Exp { $$ = new_non_terminal_node("Args", 1, $1); }
     ;
%%
#include "lex.yy.c"
yyerror(char* msg) {
    fprintf(stderr, "Error type B at Line %d: %s.\n", yylineno, msg);
}
