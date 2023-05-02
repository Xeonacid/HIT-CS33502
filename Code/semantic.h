#pragma once

#include "node.h"
#include "hash.h"

typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct LinenoList_ *LinenoList;

struct Type_ {
    enum { BASIC, ARRAY, STRUCTURE, FUNCTION } kind;
    bool is_param;
    int depth;
    union {
        // 基本类型
        enum { TYPE_INT, TYPE_FLOAT } basic;
        // 数组类型信息包括元素类型与数组大小构成
        struct {
            Type elem;
            int size;
        } array;
        // 结构体类型信息是一个链表，首个为空
        FieldList structure;
        // 函数类型信息包括返回值类型与参数类型
        struct {
            Type return_type;
            FieldList param;
            // 函数声明时的行号，定义时设置为0
            LinenoList lineno;
        } function;
    } u;
};

struct FieldList_ {
    char *name;     // 域的名字
    Type type;      // 域的类型
    FieldList tail; // 下一个域
};

struct LinenoList_ {
    int lineno;
    LinenoList tail;
};

extern HashTable *symbol_table, *struct_table;

void Program(node *program);
