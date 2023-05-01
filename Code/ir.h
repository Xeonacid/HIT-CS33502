#pragma once

#include "node.h"
#include <stdio.h>

typedef struct Operand_* Operand;
enum OperandKind { OP_VARIABLE, OP_CONSTANT, OP_ADDRESS, OP_LABEL, OP_FUNCTION };
struct Operand_ {
    enum OperandKind kind;
    const char *value, *print_name;
};

enum InterCodeKind { IR_ASSIGN, IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_LABEL, IR_FUNCTION, IR_GOTO, IR_IF_GOTO, IR_RETURN, IR_DEC, IR_ARG, IR_CALL, IR_PARAM, IR_READ, IR_WRITE, IR_GET_ADDRESS, IR_READ_ADDRESS, IR_WRITE_ADDRESS };
struct InterCode
{
    enum InterCodeKind kind;
    union {
        struct { Operand right, left; } assign;
        struct { Operand result, op1, op2; } binop;
        struct { Operand op; } unaryop;
        struct { Operand op; int size; } dec;
        struct { Operand op1, op2, label; char *relop; } if_goto;
    } u;
    struct InterCode *prev, *next;
};

extern struct InterCode *ir_head, *ir_tail;

void translate_Program(node *program);

void print_ir(struct InterCode *head, FILE *fw);
