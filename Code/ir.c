#include "ir.h"
#include "semantic.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

struct InterCode *ir_head, *ir_tail;

Type exp_array_element;

void translate_Exp(node *exp, Operand place);
void translate_Compst(node *compst);

Operand new_operand(Operand op, enum OperandKind kind, ...) {
    if (!op)
        op = (Operand)malloc(sizeof(struct Operand_));
    op->kind = kind;
    va_list ap;
    va_start(ap, kind);
    switch (kind) {
        case OP_CONSTANT: {
            int tmp = va_arg(ap, int);
            char *value = (void*)malloc(sizeof(char) * 33);
            sprintf(value, "#%d", tmp);
            op->value = value;
            break;
        }
        default:
            op->value = va_arg(ap, char*);
            break;
    }
    return op;
}

Operand zero, one;

Operand new_temp() {
    static int temp_no = 0;
    char *name = (char*)malloc(sizeof(char) * 33);
    sprintf(name, "t%d", temp_no++);
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    return new_operand(op, OP_VARIABLE, name);
}

Operand new_label() {
    static int label_no = 0;
    char *name = (char*)malloc(sizeof(char) * 33);
    sprintf(name, "label%d", label_no++);
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    return new_operand(op, OP_LABEL, name);
}

void new_ir(enum InterCodeKind kind, ...) {
    va_list ap;
    va_start(ap, kind);
    struct InterCode *ir = (struct InterCode*)malloc(sizeof(struct InterCode));
    ir->kind = kind;
    switch (kind) {
        case IR_ADD:
        case IR_ADD_ADDRESS:
        case IR_ADD_GET_ADDRESS:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            ir->u.binop.result = va_arg(ap, Operand);
            ir->u.binop.op1 = va_arg(ap, Operand);
            ir->u.binop.op2 = va_arg(ap, Operand);
            break;
        case IR_ASSIGN:
        case IR_CALL:
        case IR_GET_ADDRESS:
        case IR_READ_ADDRESS:
        case IR_WRITE_ADDRESS:
            ir->u.assign.left = va_arg(ap, Operand);
            ir->u.assign.right = va_arg(ap, Operand);
            break;
        case IR_LABEL:
        case IR_FUNCTION:
        case IR_GOTO:
        case IR_RETURN:
        case IR_ARG:
        case IR_PARAM:
        case IR_READ:
        case IR_WRITE:
            ir->u.unaryop.op = va_arg(ap, Operand);
            break;
        case IR_IF_GOTO:
            ir->u.if_goto.op1 = va_arg(ap, Operand);
            ir->u.if_goto.relop = va_arg(ap, char*);
            ir->u.if_goto.op2 = va_arg(ap, Operand);
            ir->u.if_goto.label = va_arg(ap, Operand);
            break;
        case IR_DEC:
            ir->u.dec.op = va_arg(ap, Operand);
            ir->u.dec.size = va_arg(ap, int);
            break;
        default:
            assert(0);
    }
    ir->prev = ir_tail;
    ir->next = NULL;
    if (ir_tail)
        ir_tail->next = ir;
    else
        ir_head = ir;
    ir_tail = ir;
}

int get_size(Type type) {
    switch (type->kind) {
        case BASIC:
            return 4;
        case ARRAY:
            return type->u.array.size * get_size(type->u.array.elem);
        case STRUCTURE: {
                int size = 0;
                FieldList field = type->u.structure->tail;
                while (field) {
                    size += get_size(field->type);
                    field = field->tail;
                }
                return size;
            }
        default:
            assert(0);
    }
}

void translate_Cond(node *exp, Operand label_true, Operand label_false) {
    node *exp_child = exp->child;
    node *exp_child_sibling = exp_child->sibling;

    if (!strcmp(exp_child_sibling->type, "RELOP")) {
        // Exp -> Exp RELOP Exp
        Operand t1 = new_temp();
        translate_Exp(exp_child, t1);
        Operand t2 = new_temp();
        translate_Exp(exp_child_sibling->sibling, t2);
        new_ir(IR_IF_GOTO, t1, exp_child_sibling->sval, t2, label_true);
        new_ir(IR_GOTO, label_false);
        return;
    }

    if (!strcmp(exp_child->type, "NOT")) {
        // Exp -> NOT Exp
        translate_Cond(exp_child->sibling, label_false, label_true);
        return;
    }

    if (!strcmp(exp_child_sibling->type, "AND")) {
        // Exp -> Exp AND Exp
        Operand label1 = new_label();
        translate_Cond(exp_child, label1, label_false);
        new_ir(IR_LABEL, label1);
        translate_Cond(exp_child_sibling->sibling, label_true, label_false);
        return;
    }

    if (!strcmp(exp_child_sibling->type, "OR")) {
        // Exp -> Exp OR Exp
        Operand label1 = new_label();
        translate_Cond(exp_child, label_true, label1);
        new_ir(IR_LABEL, label1);
        translate_Cond(exp_child_sibling->sibling, label_true, label_false);
        return;
    }

    Operand t1 = new_temp();
    translate_Exp(exp, t1);
    new_ir(IR_IF_GOTO, t1, "!=", zero, label_true);
    new_ir(IR_GOTO, label_false);
}

void translate_Args(node *args) {
    // Args -> Exp
    node *exp = args->child;
    if (exp->sibling) {
        // Args -> Exp COMMA Args
        translate_Args(exp->sibling->sibling);
    }
    Operand t1 = new_temp();
    translate_Exp(exp, t1);
    new_ir(IR_ARG, t1);
}

struct array_index {
    Operand t;
    struct array_index *prev, *next;
};

void translate_array(node *exp, Operand place) {
    struct array_index *head = NULL, *tail = NULL;
    int depth;
    node *cur = exp;
    // (ID LB Exp RB) LB Exp RB
    // ((Exp DOT ID) LB Exp RB) LB Exp RB
    while (cur->child->sibling && !strcmp(cur->child->sibling->type, "LB")) {
        cur = cur->child;
        struct array_index *index = (struct array_index*)malloc(sizeof(struct array_index));
        Operand t = new_temp();
        index->t = t;
        translate_Exp(cur->sibling->sibling, t);
        index->next = NULL;
        if (head) {
            tail->next = index;
            index->prev = tail;
        }
        else {
            head = index;
            index->prev = NULL;
        }
        tail = index;
        ++depth;
    }
    Operand base = new_temp();
    translate_Exp(cur, base);
    Type type = find(symbol_table, base->value);
    for (struct array_index *index = tail; index; index = index->prev) {
        Operand offset = new_temp();
        new_ir(IR_MUL, offset, index->t, new_operand(NULL, OP_CONSTANT, get_size(type->u.array.elem)));
        Operand t2 = new_temp();
        new_ir(base->kind == OP_ARRAY_OR_STRUCTURE ? IR_ADD_GET_ADDRESS : IR_ADD_ADDRESS, place, base, offset);
        base = place;
        type = type->u.array.elem;
    }
    place->kind = OP_ADDRESS;
    exp_array_element = type->u.array.elem;
    for (struct array_index *index = head; index;) {
        struct array_index *next = index->next;
        free(index);
        index = next;
    }
}

void translate_Exp(node *exp, Operand place) {
    node *exp_child = exp->child;

    if (!strcmp(exp_child->type, "INT")) {
        // Exp -> INT
        new_operand(place, OP_CONSTANT, exp_child->ival);
        return;
    }

    if (!strcmp(exp_child->type, "ID") && !exp_child->sibling) {
        // Exp -> ID
        Type type = find(symbol_table, exp_child->sval);
        new_operand(place, type->kind == BASIC ? OP_VARIABLE : (type->is_param ? OP_ADDRESS_PARAM : OP_ARRAY_OR_STRUCTURE), exp_child->sval);
        return;
    }

    node *exp_child_sibling = exp_child->sibling;

    if (!strcmp(exp_child->type, "LP")) {
        // Exp -> LP Exp RP
        translate_Exp(exp_child_sibling, place);
        return;
    }

    if (!strcmp(exp_child_sibling->type, "ASSIGNOP")) {
        // Exp -> Exp ASSIGNOP Exp
        Operand t1 = new_temp();
        translate_Exp(exp_child, t1);
        Operand t2 = new_temp();
        translate_Exp(exp_child_sibling->sibling, t2);
        new_ir(IR_ASSIGN, t1, t2);
        if (place)
            new_operand(place, OP_VARIABLE, t1->value);
        return;
    }

    if (!strcmp(exp_child_sibling->type, "PLUS")
        || !strcmp(exp_child_sibling->type, "MINUS")
        || !strcmp(exp_child_sibling->type, "STAR")
        || !strcmp(exp_child_sibling->type, "DIV")) {
        // Exp -> Exp PLUS Exp
        // Exp -> Exp MINUS Exp
        // Exp -> Exp STAR Exp
        // Exp -> Exp DIV Exp
        Operand t1 = new_temp();
        translate_Exp(exp_child, t1);
        Operand t2 = new_temp();
        translate_Exp(exp_child_sibling->sibling, t2);
        if (!strcmp(exp_child_sibling->type, "PLUS")) {
            new_ir(IR_ADD, place, t1, t2);
        } else if (!strcmp(exp_child_sibling->type, "MINUS")) {
            new_ir(IR_SUB, place, t1, t2);
        } else if (!strcmp(exp_child_sibling->type, "STAR")) {
            new_ir(IR_MUL, place, t1, t2);
        } else if (!strcmp(exp_child_sibling->type, "DIV")) {
            new_ir(IR_DIV, place, t1, t2);
        }
        return;
    }

    if(!strcmp(exp_child->type, "MINUS")) {
        // Exp -> MINUS Exp
        Operand t1 = new_temp();
        translate_Exp(exp_child_sibling, t1);
        new_ir(IR_SUB, place, zero, t1);
        return;
    }

    if (!strcmp(exp_child->type, "NOT")
        || !strcmp(exp_child_sibling->type, "RELOP")
        || !strcmp(exp_child_sibling->type, "AND")
        || !strcmp(exp_child_sibling->type, "OR")) {
        // Exp -> NOT Exp
        // Exp -> Exp RELOP Exp
        // Exp -> Exp AND Exp
        // Exp -> Exp OR Exp
        Operand label1 = new_label();
        Operand label2 = new_label();
        new_operand(place, OP_CONSTANT, 0);
        translate_Cond(exp, label1, label2);
        new_ir(IR_LABEL, label1);
        new_ir(IR_ASSIGN, place, one);
        new_ir(IR_LABEL, label2);
        return;
    }

    if (exp_child_sibling && !strcmp(exp_child_sibling->type, "LB")) {
        // Exp -> Exp LB Exp RB
        translate_array(exp, place);
        return;
    }

    if (!strcmp(exp_child_sibling->type, "DOT")) {
        // Exp -> Exp DOT ID
        Operand t1 = new_temp();
        translate_Exp(exp_child, t1);
        Type type = find(symbol_table, t1->value);
        if (!type)
            // Exp -> Exp LB Exp RB DOT ID
            type = exp_array_element;
        FieldList field = type->u.structure;
        node *id = exp_child_sibling->sibling;
        int offset = 0;
        while (field) {
            if (field->name) {
                if (!strcmp(field->name, id->sval))
                    break;
                offset += get_size(field->type);
            }
            field = field->tail;
        }
        Operand t2 = new_temp();
        new_operand(t2, OP_CONSTANT, offset);
        new_ir(t1->kind == OP_ARRAY_OR_STRUCTURE ? IR_ADD_GET_ADDRESS : IR_ADD_ADDRESS, place, t1, t2);
        place->kind = OP_ADDRESS;
        if (field->type->kind == ARRAY)
            // Exp -> Exp DOT ID LB Exp RB
            place->value = id->sval;
        return;
    }

    if (!strcmp(exp_child->type, "ID")) {
        // Exp -> ID LP RP
        if (!strcmp(exp_child->sval, "read")) {
            new_ir(IR_READ, place ? place : new_temp());
            return;
        }

        if (!strcmp(exp_child_sibling->sibling->type, "Args")) {
            // Exp -> ID LP Args RP
            if (!strcmp(exp_child->sval, "write")) {
                Operand t1 = new_temp();
                translate_Exp(exp_child_sibling->sibling->child, t1);
                new_ir(IR_WRITE, t1);
                return;
            }
            translate_Args(exp_child_sibling->sibling);
        }
        Operand func = new_operand(NULL, OP_FUNCTION, exp_child->sval);
        new_ir(IR_CALL, place, func);
        return;
    }
}

void translate_VarDec(node *vardec, Operand place) {
    if (!strcmp(vardec->child->type, "ID")) {
        node *id = vardec->child;
        Type type = find(symbol_table, id->sval);
        switch (type->kind) {
            case BASIC:
                if (place)
                    new_operand(place, OP_VARIABLE, id->sval);
                break;
            case ARRAY:
            case STRUCTURE:
                Operand op = new_operand(place, OP_ARRAY_OR_STRUCTURE, id->sval);
                new_ir(IR_DEC, op, get_size(type));
                break;
            default:
                assert(0);
        }
        return;
    }

    translate_VarDec(vardec->child, place);
}

void translate_Dec(node *dec) {
    node *vardec = dec->child;
    node *assignop = vardec->sibling;
    if (!assignop) {
        // Dec -> VarDec
        translate_VarDec(vardec, NULL);
        return;
    }

    // Dec -> VarDec ASSIGNOP Exp
    Operand t1 = new_temp();
    translate_VarDec(vardec, t1);
    Operand t2 = new_temp();
    translate_Exp(assignop->sibling, t2);
    new_ir(IR_ASSIGN, t1, t2);
}

void translate_DecList(node *declist) {
    node *dec = declist->child;
    node *comma = dec->sibling;
    // DecList -> Dec
    translate_Dec(dec);
    if (comma)
        // DecList -> Dec COMMA DecList
        translate_DecList(comma->sibling);
}

void translate_DefList(node *deflist) {
    // DefList ->
    node *def = deflist->child;
    if (def) {
        // DefList -> Def DefList
        // Def -> Specifier DecList SEMI
        translate_DecList(def->child->sibling);
        translate_DefList(def->sibling);
    }
}

void translate_Stmt(node *stmt) {
    node *child = stmt->child;

    if (!strcmp(child->type, "Exp")) {
        // Stmt -> Exp SEMI
        translate_Exp(child, NULL);
        return;
    }

    if (!strcmp(child->type, "CompSt")) {
        // Stmt -> CompSt
        translate_Compst(child);
        return;
    }

    if (!strcmp(child->type, "RETURN")) {
        // Stmt -> RETURN Exp SEMI
        Operand t1 = new_temp();
        translate_Exp(child->sibling, t1);
        new_ir(IR_RETURN, t1);
        return;
    }

    if (!strcmp(child->type, "IF")) {
        Operand label1 = new_label();
        Operand label2 = new_label();
        node *exp = child->sibling->sibling;
        translate_Cond(exp, label1, label2);
        new_ir(IR_LABEL, label1);
        node *if_stmt = exp->sibling->sibling;
        translate_Stmt(if_stmt);

        if (!if_stmt->sibling)
            // Stmt -> IF LP Exp RP Stmt
            new_ir(IR_LABEL, label2);
        else {
            // Stmt -> IF LP Exp RP Stmt ELSE Stmt
            Operand label3 = new_label();
            new_ir(IR_GOTO, label3);
            new_ir(IR_LABEL, label2);
            translate_Stmt(if_stmt->sibling->sibling);
            new_ir(IR_LABEL, label3);
        }
        return;
    }

    if (!strcmp(child->type, "WHILE")) {
        // Stmt -> WHILE LP Exp RP Stmt
        Operand label1 = new_label();
        Operand label2 = new_label();
        Operand label3 = new_label();
        new_ir(IR_LABEL, label1);
        node *exp = child->sibling->sibling;
        translate_Cond(exp, label2, label3);
        new_ir(IR_LABEL, label2);
        translate_Stmt(exp->sibling->sibling);
        new_ir(IR_GOTO, label1);
        new_ir(IR_LABEL, label3);
        return;
    }
}

void translate_StmtList(node *stmtlist) {
    // StmtList ->
    node *stmt = stmtlist->child;
    if (stmt) {
        // StmtList -> Stmt StmtList
        translate_Stmt(stmt);
        translate_StmtList(stmt->sibling);
    }
}

void translate_Compst(node *compst) {
    node *deflist = compst->child->sibling;
    node *stmtlist = deflist->sibling;
    translate_DefList(deflist);
    translate_StmtList(stmtlist);
}

void translate_FunDec(node *fundec) {
    // FunDec -> ID LP RP
    node *id = fundec->child;
    new_ir(IR_FUNCTION, new_operand(NULL, OP_FUNCTION, id->sval));
    if (id->sibling->sibling->sibling) {
        // FunDec -> ID LP VarList RP
        Type type = find(symbol_table, id->sval);
        FieldList field = type->u.function.param->tail;
        while (field) {
            new_ir(IR_PARAM, new_operand(NULL, OP_VARIABLE, field->name));
            field = field->tail;
        }
    }
}

void translate_ExtDef(node *extdef) {
    // ExtDef -> Specifier FunDec CompSt
    node *fundec = extdef->child->sibling;
    if (!strcmp(fundec->type, "FunDec")) {
        translate_FunDec(fundec);
        translate_Compst(fundec->sibling);
    }
}

void translate_ExtDefList(node *extdeflist) {
    // ExtDefList ->
    node *extdef = extdeflist->child;
    if (extdef) {
        // ExtDefList -> ExtDef ExtDefList
        translate_ExtDef(extdef);
        translate_ExtDefList(extdef->sibling);
    }
}

void translate_Program(node *program) {
    zero = new_operand(NULL, OP_CONSTANT, 0);
    one = new_operand(NULL, OP_CONSTANT, 1);

    // Program -> ExtDefList
    translate_ExtDefList(program->child);
}

const char*deref_name(Operand op) {
    if (op->kind != OP_ADDRESS)
        return op->value;
    const char *tmp = op->value;
    char *deref_name = malloc(strlen(tmp) + 2);
    sprintf(deref_name, "*%s", tmp);
    return deref_name;
}

const char*get_addr_name(Operand op) {
    if (op->kind != OP_ARRAY_OR_STRUCTURE)
        return op->value;
    const char *tmp = op->value;
    char *deref_name = malloc(strlen(tmp) + 2);
    sprintf(deref_name, "&%s", tmp);
    return deref_name;
}

void print_ir(struct InterCode *head, FILE *fp) {
    while (head) {
        switch (head->kind) {
            case IR_ASSIGN:
                fprintf(fp, "%s := %s\n", deref_name(head->u.assign.left), deref_name(head->u.assign.right));
                break;
            case IR_ADD:
                fprintf(fp, "%s := %s + %s\n", head->u.binop.result->value,
                        deref_name(head->u.binop.op1), deref_name(head->u.binop.op2));
                break;
            case IR_ADD_ADDRESS:
                fprintf(fp, "%s := %s + %s\n", head->u.binop.result->value, head->u.binop.op1->value, head->u.binop.op2->value);
                break;
            case IR_ADD_GET_ADDRESS:
                fprintf(fp, "%s := %s + %s\n", head->u.binop.result->value,
                        get_addr_name(head->u.binop.op1), get_addr_name(head->u.binop.op2));
                break;
            case IR_SUB:
                fprintf(fp, "%s := %s - %s\n", head->u.binop.result->value,
                        deref_name(head->u.binop.op1), deref_name(head->u.binop.op2));
                break;
            case IR_MUL:
                fprintf(fp, "%s := %s * %s\n", head->u.binop.result->value,
                        deref_name(head->u.binop.op1), deref_name(head->u.binop.op2));
                break;
            case IR_DIV:
                fprintf(fp, "%s := %s / %s\n", head->u.binop.result->value,
                        deref_name(head->u.binop.op1), deref_name(head->u.binop.op2));
                break;
            case IR_LABEL:
                fprintf(fp, "LABEL %s :\n", head->u.unaryop.op->value);
                break;
            case IR_FUNCTION:
                fprintf(fp, "FUNCTION %s :\n", head->u.unaryop.op->value);
                break;
            case IR_GOTO:
                fprintf(fp, "GOTO %s\n", head->u.unaryop.op->value);
                break;
            case IR_IF_GOTO:
                fprintf(fp, "IF %s %s %s GOTO %s\n", deref_name(head->u.if_goto.op1),
                        head->u.if_goto.relop, deref_name(head->u.if_goto.op2),
                        head->u.if_goto.label->value);
                break;
            case IR_RETURN:
                fprintf(fp, "RETURN %s\n", deref_name(head->u.unaryop.op));
                break;
            case IR_DEC:
                fprintf(fp, "DEC %s %d\n", head->u.dec.op->value,
                        head->u.dec.size);
                break;
            case IR_ARG:
                fprintf(fp, "ARG %s\n", get_addr_name(head->u.unaryop.op));
                break;
            case IR_CALL:
                fprintf(fp, "%s := CALL %s\n", head->u.assign.left->value,
                        head->u.assign.right->value);
                break;
            case IR_PARAM:
                fprintf(fp, "PARAM %s\n", head->u.unaryop.op->value);
                break;
            case IR_READ:
                fprintf(fp, "READ %s\n", deref_name(head->u.unaryop.op));
                break;
            case IR_WRITE:
                fprintf(fp, "WRITE %s\n", deref_name(head->u.unaryop.op));
                break;
            case IR_GET_ADDRESS:
                fprintf(fp, "%s := &%s\n", head->u.assign.left->value,
                        head->u.assign.right->value);
                break;
            case IR_READ_ADDRESS:
                fprintf(fp, "%s := *%s\n", head->u.assign.left->value,
                        head->u.assign.right->value);
                break;
            case IR_WRITE_ADDRESS:
                fprintf(fp, "*%s := %s\n", head->u.assign.left->value,
                        head->u.assign.right->value);
                break;
        }
        head = head->next;
    }
}
