#include "semantic.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

enum DecType {
    VAR,
    STRUCT,
    FUNC_DEC,
    FUNC_DEF,
};

HashTable *symbol_table, *struct_table;
int depth;

void CompSt(node *compst, Type return_type);
Type Exp(node *exp);
Type Specifier(node *specifier);

bool is_same_type(Type type1, Type type2);

bool is_same_fieldlist(FieldList field1, FieldList field2) {
    if (!field1)
        return !field2;
    if (!field2)
        return false;
    while (field1 && field2) {
        if (!is_same_type(field1->type, field2->type))
            return false;
        field1 = field1->tail;
        field2 = field2->tail;
    }
    return !field1 && !field2;
}

bool is_same_type(Type type1, Type type2) {
    if (!type1)
        return !type2;
    if (!type2)
        return false;
    if (type1->kind != type2->kind)
        return false;
    switch (type1->kind) {
        case BASIC:
            return type1->u.basic == type2->u.basic;
        case ARRAY:
            return is_same_type(type1->u.array.elem, type2->u.array.elem);
        case STRUCTURE:
            return is_same_fieldlist(type1->u.structure, type2->u.structure);
        case FUNCTION:
            return false;
    }
}

void check_undefined_function() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        HashNode *hashnode = symbol_table->row_heads[i];
        while (hashnode) {
            Type type = (Type)hashnode->value;
            if (type->kind != FUNCTION)
                goto next;
            LinenoList linenolist = type->u.function.lineno;
            while (linenolist) {
                if (linenolist->lineno)
                    fprintf(stderr, "Error type 18 at Line %d: Undefined function \"%s\".\n", linenolist->lineno, hashnode->key);
                linenolist = linenolist->tail;
            }
            next:
            hashnode = hashnode->next;
        }
    }
}

void leave_scope() {
    remove_column(symbol_table, depth);
    remove_column(struct_table, depth--);
}

FieldList Args(node *args) {
    // Args -> Exp COMMA Args
    FieldList head = (FieldList)malloc(sizeof(struct FieldList_)), tail = head;
    for(;;) {
        Type type = Exp(args->child);
        if (!type)
            return NULL;
        FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
        field->type = type;
        tail = tail->tail = field;
        if (!args->child->sibling)
            // Args -> Exp
            break;
        args = args->child->sibling->sibling;
    }
    return head;
}

Type Exp(node *exp) {
    if (!strcmp(exp->child->type, "ID") && !exp->child->sibling) {
        // Exp -> ID
        exp->is_rvalue = false;
        Type type = find(symbol_table, exp->child->sval);
        if (!type) {
            fprintf(stderr, "Error type 1 at Line %d: Undefined variable \"%s\".\n", exp->lineno, exp->child->sval);
            return NULL;
        }
        return type;
    } else if (!strcmp(exp->child->type, "INT")) {
        // Exp -> INT
        exp->is_rvalue = true;
        Type type = (Type)malloc(sizeof(struct Type_));
        type->kind = BASIC;
        type->depth = depth;
        type->u.basic = TYPE_INT;
        return type;
    } else if (!strcmp(exp->child->type, "FLOAT")) {
        // Exp -> FLOAT
        exp->is_rvalue = true;
        Type type = (Type)malloc(sizeof(struct Type_));
        type->kind = BASIC;
        type->depth = depth;
        type->u.basic = TYPE_FLOAT;
        return type;
    } else if (!strcmp(exp->child->sibling->type, "ASSIGNOP")) {
        // Exp -> Exp ASSIGNOP Exp
        exp->is_rvalue = false;
        node *left = exp->child;
        Type left_type = Exp(left);
        if (left->is_rvalue) {
            fprintf(stderr, "Error type 6 at Line %d: The left-hand side of an assignment must be a variable.\n", exp->lineno);
            return NULL;
        }
        Type right_type = Exp(left->sibling->sibling);
        if (left_type && !is_same_type(left_type, right_type)) {
            fprintf(stderr, "Error type 5 at Line %d: Type mismatched for assignment.\n", exp->lineno);
            return NULL;
        }
        return left_type;
    } else if (!strcmp(exp->child->sibling->type, "AND")
               || !strcmp(exp->child->sibling->type, "OR")
               || !strcmp(exp->child->sibling->type, "RELOP")
               || !strcmp(exp->child->sibling->type, "PLUS")
               || !strcmp(exp->child->sibling->type, "MINUS")
               || !strcmp(exp->child->sibling->type, "STAR")
               || !strcmp(exp->child->sibling->type, "DIV")) {
        // Exp -> Exp AND Exp
        // Exp -> Exp OR Exp
        // Exp -> Exp RELOP Exp
        // Exp -> Exp PLUS Exp
        // Exp -> Exp MINUS Exp
        // Exp -> Exp STAR Exp
        // Exp -> Exp DIV Exp
        exp->is_rvalue = true;
        Type left_type = Exp(exp->child), right_type = Exp(exp->child->sibling->sibling);
        if (left_type && right_type && (left_type->kind != BASIC || right_type->kind != BASIC || left_type->u.basic != right_type->u.basic)) {
            fprintf(stderr, "Error type 7 at Line %d: Type mismatched for operands.\n", exp->lineno);
            return NULL;
        }
        return left_type;
    } else if (!strcmp(exp->child->type, "LP")) {
        // Exp -> LP Exp RP
        node *exp_child = exp->child->sibling;
        Type type = Exp(exp_child);
        exp->is_rvalue = exp_child->is_rvalue;
        return type;
    } else if (!strcmp(exp->child->type, "MINUS") || !strcmp(exp->child->type, "NOT")) {
        // Exp -> MINUS Exp
        // Exp -> NOT Exp
        exp->is_rvalue = true;
        Type type = Exp(exp->child->sibling);
        if (type->kind != BASIC) {
            fprintf(stderr, "Error type 7 at Line %d: Type mismatched for operands.\n", exp->lineno);
            return NULL;
        }
        return type;
    } else if (!strcmp(exp->child->sibling->type, "LP")) {
        // Exp -> ID LP Args RP
        // Exp -> ID LP RP
        exp->is_rvalue = true;
        node *id = exp->child;
        if (!strcmp(id->sval, "read") || !strcmp(id->sval, "write")) {
            // skip check for lab3
            Type type = (Type)malloc(sizeof(struct Type_));
            type->kind = BASIC;
            type->depth = depth;
            type->u.basic = TYPE_INT;
            return type;
        }
        node *args = id->sibling->sibling;
        Type func_type = find(symbol_table, id->sval);
        if (!func_type) {
            fprintf(stderr, "Error type 2 at Line %d: Undefined function \"%s\".\n", exp->lineno, id->sval);
            return NULL;
        }
        if (func_type->kind != FUNCTION) {
            fprintf(stderr, "Error type 11 at Line %d: \"%s\" is not a function.\n", exp->lineno, id->sval);
            return NULL;
        }
        FieldList param = func_type->u.function.param, arg = NULL;
        if (!strcmp(args->type, "Args"))
            // Exp -> ID LP Args RP
            arg = Args(args);
        if (!is_same_fieldlist(param, arg)) {
            fprintf(stderr, "Error type 9 at Line %d: Function \"%s\" is not applicable for arguments.\n", exp->lineno, id->sval); // TODO
            return NULL;
        }
        return func_type->u.function.return_type;
    } else if (!strcmp(exp->child->sibling->type, "LB")) {
        // Exp -> Exp LB Exp RB
        node *exp_array = exp->child;
        Type array_type = Exp(exp_array);
        exp->is_rvalue = exp_array->is_rvalue;
        Type index_type = Exp(exp_array->sibling->sibling);
        if (array_type->kind != ARRAY) {
            fprintf(stderr, "Error type 10 at Line %d: is not an array.\n", exp->lineno);
            return NULL;
        }
        if (index_type->kind != BASIC || index_type->u.basic != TYPE_INT)
            fprintf(stderr, "Error type 12 at Line %d: is not an integer.\n", exp->lineno);
        return array_type->u.array.elem;
    } else if (!strcmp(exp->child->sibling->type, "DOT")) {
        // Exp -> Exp DOT ID
        node *exp_child = exp->child;
        Type struct_type = Exp(exp_child);
        exp->is_rvalue = exp_child->is_rvalue;
        if (struct_type->kind != STRUCTURE) {
            fprintf(stderr, "Error type 13 at Line %d: Illegal use of \".\".\n", exp->lineno);
            return NULL;
        }
        FieldList field = struct_type->u.structure->tail;
        node *id = exp->child->sibling->sibling;
        while (field) {
            if (!strcmp(field->name, id->sval))
                return field->type;
            field = field->tail;
        }
        fprintf(stderr, "Error type 14 at Line %d: Non-existent field \"%s\".\n", exp->lineno, exp->child->sibling->sibling->sval);
        return NULL;
    } 
    assert(false);
    return NULL;
}

void VarDec(node *vardec, Type type, enum DecType dectype, Type exp_type, FieldList *fieldlist, FieldList *fieldlist_tail) {
    if (!strcmp(vardec->child->type, "ID")) {
        // VarDec -> ID
        node *id = vardec->child;
        if (dectype == VAR && exp_type && !is_same_type(type, exp_type)) {
            fprintf(stderr, "Error type 5 at Line %d: Type mismatched for assignment.\n", vardec->lineno);
            return;
        }
        if (dectype == VAR || dectype == FUNC_DEF) {
            // Compst -> LC DefList StmtList RC
            // FunDec -> ID LP VarList RP
            Type tmp;
            if ((tmp = find(symbol_table, id->sval)) && tmp->depth == depth || (tmp = find(struct_table, id->sval)) && tmp->depth == depth) {
                fprintf(stderr, "Error type 3 at Line %d: Redefined variable \"%s\".\n", vardec->lineno, id->sval);
                return;
            }
            insert(symbol_table, id->sval, type, depth);
        }
        if (dectype == STRUCT || dectype == FUNC_DEC || dectype == FUNC_DEF) {
            // StructSpecifier -> STRUCT OptTag LC DefList RC
            // FunDec -> ID LP VarList RP
            FieldList tmp = (*fieldlist)->tail;
            while (tmp) {
                if (!strcmp(tmp->name, id->sval)) {
                    fprintf(stderr, dectype == STRUCT ? "Error type 15 at Line %d: Redefined field \"%s\".\n": "Error type 3 at Line %d: Redefined variable \"%s\".\n", vardec->lineno, id->sval);
                    return;
                }
                tmp = tmp->tail;
            }
            FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
            field->name = id->sval;
            field->type = type;
            field->tail = NULL;
            *fieldlist_tail = (*fieldlist_tail)->tail = field;
        }
    } else {
        // VarDec -> VarDec LB INT RB
        node *int_node = vardec->child->sibling->sibling;
        Type array_type = (Type)malloc(sizeof(struct Type_));
        array_type->kind = ARRAY;
        array_type->depth = depth;
        array_type->u.array.elem = type;
        array_type->u.array.size = int_node->ival;
        VarDec(vardec->child, array_type, dectype, exp_type, fieldlist, fieldlist_tail);
    }
}

void Dec(node *dec, Type type, enum DecType dectype, FieldList *fieldlist, FieldList *fieldlist_tail) {
    // Dec -> VarDec
    node *vardec = dec->child;
    Type exp_type = NULL;
    // Dec -> VarDec ASSIGNOP Exp
    if (vardec->sibling) {
        // StructSpecifier -> STRUCT Tag LC DefList RC
        if (dectype == STRUCT) {
            fprintf(stderr, "Error type 15 at Line %d: Initialize field \"%s\".\n", dec->lineno, vardec->child->sval);
            return;
        }
        exp_type = Exp(vardec->sibling->sibling);
    }
    VarDec(vardec, type, dectype, exp_type, fieldlist, fieldlist_tail);
}

void DecList(node *declist, Type type, enum DecType dectype, FieldList *fieldlist, FieldList *fieldlist_tail) {
    // DecList -> Dec
    node *dec = declist->child;
    Dec(dec, type, dectype, fieldlist, fieldlist_tail);

    // DecList -> Dec COMMA DecList
    if (dec->sibling)
        DecList(dec->sibling->sibling, type, dectype, fieldlist, fieldlist_tail);
}

void Def(node *def, enum DecType dectype, FieldList *fieldlist, FieldList *fieldlist_tail) {
    // Def -> Specifier DecList SEMI
    Type type = Specifier(def->child);
    DecList(def->child->sibling, type, dectype, fieldlist, fieldlist_tail);
}

void DefList(node *deflist, enum DecType dectype, FieldList *fieldlist, FieldList *fieldlist_tail) {
    if (!deflist->child)
        // DefList ->
        return;

    // DefList -> Def DefList
    Def(deflist->child, dectype, fieldlist, fieldlist_tail);
    DefList(deflist->child->sibling, dectype, fieldlist, fieldlist_tail);
}

void Stmt(node *stmt, Type return_type) {
    // Stmt -> Exp SEMI
    if (!strcmp(stmt->child->type, "Exp"))
        Exp(stmt->child);

    // Stmt -> CompSt
    else if (!strcmp(stmt->child->type, "CompSt")) {
        ++depth;
        CompSt(stmt->child, return_type);
        // leave_scope(); // for lab3
    }

    // Stmt -> RETURN Exp SEMI
    else if (!strcmp(stmt->child->type, "RETURN")) {
        Type exp_type = Exp(stmt->child->sibling);
        if (!is_same_type(return_type, exp_type))
            fprintf(stderr, "Error type 8 at Line %d: Type mismatched for return.\n", stmt->lineno);
    }

    // Stmt -> IF LP Exp RP Stmt
    else if (!strcmp(stmt->child->type, "IF")) {
        node *exp = stmt->child->sibling->sibling;
        Exp(exp);
        node *stmt_if = exp->sibling->sibling;
        Stmt(stmt_if, return_type);

        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        if (stmt_if->sibling)
            Stmt(stmt_if->sibling->sibling, return_type);
    }

    // Stmt -> WHILE LP Exp RP Stmt
    else if (!strcmp(stmt->child->type, "WHILE")) {
        node *exp = stmt->child->sibling->sibling;
        Exp(exp);
        Stmt(exp->sibling->sibling, return_type);
    }
}

void StmtList(node *stmtlist, Type return_type) {
    node *stmt = stmtlist->child;
    // StmtList ->
    if (stmt) {
        // StmtList -> Stmt StmtList
        Stmt(stmt, return_type);
        StmtList(stmt->sibling, return_type);
    }
}

void CompSt(node *compst, Type return_type) {
    // CompSt -> LC DefList StmtList RC
    node *deflist = compst->child->sibling;
    DefList(deflist, VAR, NULL, NULL);
    StmtList(deflist->sibling, return_type);
}

Type Tag(node *tag) {
    // Tag -> ID
    node *id = tag->child;
    Type type = find(struct_table, id->sval);
    if(!type)
        fprintf(stderr, "Error type 17 at Line %d: Undefined structure \"%s\".\n", tag->lineno, id->sval);
    return type;
}

char *OptTag(node *opttag) {
    // OptTag -> ID
    // OptTag ->
    node *id = opttag->child;
    return id ? id->sval : NULL;
}

Type StructSpecifier(node *structspecifier) {
    if (!strcmp(structspecifier->child->sibling->type, "OptTag")) {
        // StructSpecifier -> STRUCT OptTag LC DefList RC
        node *opttag = structspecifier->child->sibling;
        char *struct_name = OptTag(opttag);
        if (struct_name && (find(struct_table, struct_name) || find(symbol_table, struct_name))) {
            fprintf(stderr, "Error type 16 at Line %d: Duplicated name \"%s\".\n", opttag->lineno, struct_name);
            return NULL;
        }
        Type type = (Type)malloc(sizeof(struct Type_));
        type->kind = STRUCTURE;
        type->depth = depth;
        FieldList fieldlist = (FieldList)malloc(sizeof(struct FieldList_)), fieldlist_tail = fieldlist;
        type->u.structure = fieldlist;
        node *deflist = opttag->sibling->sibling;
        DefList(deflist, STRUCT, &fieldlist, &fieldlist_tail);
        if (struct_name)
            insert(struct_table, struct_name, type, depth);
        return type;
    } else {
        // StructSpecifier -> STRUCT Tag
        node *tag = structspecifier->child->sibling;
        return Tag(tag);
    }
}

Type Specifier(node *specifier) {
    if (!strcmp(specifier->child->type, "TYPE")) {
        // Specifier -> TYPE
        if(!strcmp(specifier->child->sval, "int")) {
            Type type = (Type)malloc(sizeof(struct Type_));
            type->kind = BASIC;
            type->depth = depth;
            type->u.basic = TYPE_INT;
            return type;
        } else {
            Type type = (Type)malloc(sizeof(struct Type_));
            type->kind = BASIC;
            type->depth = depth;
            type->u.basic = TYPE_FLOAT;
            return type;
        }
    } else
        // Specifier -> StructSpecifier
        return StructSpecifier(specifier->child);
}

void ParamDec(node *paramdec, enum DecType dectype, FieldList *fieldlist, FieldList *fieldlist_tail) {
    // ParamDec -> Specifier VarDec
    Type type = Specifier(paramdec->child);
    VarDec(paramdec->child->sibling, type, dectype, NULL, fieldlist, fieldlist_tail);
}

FieldList VarList(node *varlist, enum DecType dectype) {
    node *paramdec = varlist->child;
    // VarList -> ParamDec
    FieldList fieldlist = (FieldList)malloc(sizeof(struct FieldList_)), fieldlist_tail = fieldlist;
    ParamDec(paramdec, dectype, &fieldlist, &fieldlist_tail);

    // VarList -> ParamDec COMMA VarList
    if (paramdec->sibling)
        fieldlist_tail->tail = VarList(paramdec->sibling->sibling, dectype)->tail;

    return fieldlist;
}

Type FunDec(node *fundec, Type return_type, int dec_lineno) {
    // FunDec -> ID LP RP
    node *id = fundec->child;
    Type dec_before = find(symbol_table, id->sval);
    if (dec_before) {
        if (dec_before->kind != FUNCTION) {
            fprintf(stderr, "Error type 4 at Line %d: The symbol \"%s\" is not a function.\n", fundec->lineno, id->sval);
            return NULL;
        }
        if (!dec_before->u.function.lineno) {
            fprintf(stderr, "Error type 4 at Line %d: Redefined function \"%s\".\n", fundec->lineno, id->sval);
            return NULL;
        }
        if (!is_same_type(dec_before->u.function.return_type, return_type)) {
            fprintf(stderr, "Error type 19 at Line %d: Inconsistent declaration of function \"%s\".\n", fundec->lineno, id->sval);
            return NULL;
        }
    }
    Type type = (Type)malloc(sizeof(struct Type_));
    type->kind = FUNCTION;
    type->depth = depth;
    type->u.function.return_type = return_type;
    ++depth;
    // FunDec -> ID LP VarList RP
    type->u.function.param = id->sibling->sibling->sibling ? VarList(id->sibling->sibling, dec_lineno ? FUNC_DEC : FUNC_DEF) : NULL;
    if (dec_before) {
        if(!is_same_fieldlist(dec_before->u.function.param, type->u.function.param)) {
            fprintf(stderr, "Error type 19 at Line %d: Inconsistent declaration of function \"%s\".\n", fundec->lineno, id->sval);
            free(type);
            return NULL;
        }
        if (!dec_lineno)
            dec_before->u.function.lineno = NULL;
        else {
            LinenoList linenolist = (LinenoList)malloc(sizeof(struct LinenoList_));
            linenolist->lineno = dec_lineno;
            linenolist->tail = dec_before->u.function.lineno;
            dec_before->u.function.lineno = linenolist;
        }
    }
    else {
        if (!dec_lineno)
            type->u.function.lineno = NULL;
        else {
            LinenoList linenolist = (LinenoList)malloc(sizeof(struct LinenoList_));
            linenolist->lineno = dec_lineno;
            linenolist->tail = NULL;
            type->u.function.lineno = linenolist;
        }
        insert(symbol_table, id->sval, type, depth - 1);
    }
    return type;
}

void ExtDecList(node *extdeclist, Type type) {
    // ExtDecList -> VarDec
    node *vardec = extdeclist->child;
    VarDec(vardec, type, VAR, NULL, NULL, NULL);

    // ExtDecList -> VarDec COMMA ExtDecList
    if (vardec->sibling)
        ExtDecList(vardec->sibling->sibling, type);
}

void ExtDef(node *extdef) {
    // ExtDef -> Specifier ExtDecList SEMI
    if (!strcmp(extdef->child->sibling->type, "ExtDecList")) {
        node *specifier = extdef->child;
        Type type = Specifier(specifier);
        ExtDecList(specifier->sibling, type);
    }

    // ExtDef -> Specifier SEMI
    else if (!strcmp(extdef->child->sibling->type, "SEMI")) {
        node *specifier = extdef->child;
        Specifier(specifier);
    }

    // ExtDef -> Specifier FunDec CompSt
    // ExtDef -> Specifier FunDec SEMI
    else if (!strcmp(extdef->child->sibling->type, "FunDec")) {
        node *specifier = extdef->child;
        Type return_type = Specifier(specifier);
        node *fundec = extdef->child->sibling;
        bool is_def = !strcmp(fundec->sibling->type, "CompSt");
        FunDec(fundec, return_type, is_def ? 0 : extdef->lineno);
        if (is_def)
            CompSt(fundec->sibling, return_type);
        // leave_scope(); // for lab3
    }
}

void ExtDefList(node *extdeflist) {
    node *extdef = extdeflist->child;
    // ExtDefList ->
    if (!extdef)
        return;

    // ExtDefList -> ExtDef ExtDefList
    ExtDef(extdef);
    ExtDefList(extdef->sibling);
}

void Program(node *program) {
    symbol_table = create_table();
    struct_table = create_table();
    
    // Program -> ExtDefList
    ExtDefList(program->child);

    check_undefined_function();
}
