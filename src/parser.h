#ifndef PARSER_H_
#define PARSER_H_

#include "panic.h"
#include "reader.h"
#include "lexer.h"

typedef long long     i64;
typedef unsigned long u64;
typedef double        f64;

typedef struct Context Context;
typedef struct Module Module;

struct Module {
    Arena    a;
    CString  name;
    Map      typetable;
    Map      callstack;
    Context  *stmt;
};

struct Context {
    alf_byte    type;     // For detecting if context is function body
    alf_uint    datasize; 
    alf_uint    codesize;
    alf_uint    dataptr;
    alf_uint    codeptr;
    Module      *module;
    Instruction *code;    // Code placeholder for executing statements
    StkVal      *data;    // Data for instructions
};

typedef struct {
    alf_byte used:1;
    alf_uint type;    // Return type
    Address  addr;
    Context  *c;
} Fundef;

typedef struct Expr Expr;

typedef enum {
    EXPR_NONE=0,
    EXPR_INT,
    EXPR_FLT,
    EXPR_STR,
    EXPR_UINT,
    EXPR_FUNCALL,
    EXPR_OPTION
} expr_t;

typedef struct {
    alf_byte bltin:1;
    CString  name;
    size_t   count;
    size_t   capacity;
    Expr     *items;     // Argumets
} Funcall;

struct Expr {
    expr_t t; 
    union {
        i64     i;
        u64     u;
        f64     f;
        char    c;
        String  *s;
        Funcall *fc;
    };
};

#define ExprNone (Expr) { .t = EXPR_NONE }


#define StkInt(v)   (StkVal) { .i = (v) }
#define StkStr(v)   (StkVal) { .s = (v) }
#define StkFlt(v)   (StkVal) { .f = (v) }
#define StkUint(v)  (StkVal) { .u = (v) }
#define StkChar(v)  (StkVal) { .c = (v) }
#define StkPtr(v)   (StkVal) { .p = (v) }

extern void mainfunc(Alf_State *alf, Reader *r);
extern Expr expr(Arena *a, Lexer *l);

#endif // PARSER_H_
