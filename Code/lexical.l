%option yylineno

INT 0|[1-9][0-9]*
FLOAT {INT}\.[0-9]+
ID [_a-zA-Z][0-9_a-zA-Z]*
RELOP >|<|>=|<=|==|!=
TYPE int|float

ws [ \t]
newline \n|\r\n
%%
{ws}|{newline}
{INT} { printf("INT\n"); }
{FLOAT} { printf("FLOAT\n"); }
{TYPE} { printf("TYPE\n"); }
";" { printf("SEMI\n"); }
"," { printf("COMMA\n"); }
"=" { printf("ASSIGNOP\n"); }
{RELOP} { printf("RELOP\n"); }
"+" { printf("PLUS\n"); }
"-" { printf("MINUS\n"); }
"*" { printf("STAR\n"); }
"/" { printf("DIV\n"); }
"&&" { printf("AND\n"); }
"||" { printf("OR\n"); }
"." { printf("DOT\n"); }
"!" { printf("NOT\n"); }
"(" { printf("LP\n"); }
")" { printf("RP\n"); }
"[" { printf("LB\n"); }
"]" { printf("RB\n"); }
"{" { printf("LC\n"); }
"}" { printf("RC\n"); }
"struct" { printf("STRUCT\n"); }
"return" { printf("RETURN\n"); }
"if" { printf("IF\n"); }
"else" { printf("ELSE\n"); }
"while" { printf("WHILE\n"); }
{ID} { printf("ID\n"); }
. { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }
