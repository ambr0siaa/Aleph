#ifndef COMMON_H_
#define COMMON_H_

#include "hashmap.h"
#include "arena.h"
#include "str.h"

typedef size_t        alf_uint;
typedef alf_uint      Address;
typedef unsigned char alf_byte;
typedef long int      alf_int;
typedef double        alf_flt;
typedef unsigned char alf_bool;

#define ALF_API  extern
#define ALF_FUNC static inline

#define ALF_STATUS_OK  0
#define ALF_STATUS_FCK 1

typedef struct {
    Address     line;
    Address     offset;
    const char  *file;
} Location;

// 48
typedef enum {
    OP_POP,
    OP_STR,
    OP_PULL,
    OP_BLTIN,
    OP_JZ,
    OP_JNZ,
    OP_CALL,
    OP_RET,
    OP_HLT,
    OP_ADDI,
    OP_SUBI,
    OP_DIVI,
    OP_MULI,
    OP_ADDU,
    OP_SUBU,
    OP_DIVU,
    OP_MULU,
    OP_ADDF,
    OP_SUBF,
    OP_DIVF,
    OP_MULF,
    OP_ANDI,
    OP_ORI,
    OP_XORI,
    OP_NOTI,
    OP_SHRI,
    OP_SHLI,
    OP_ANDU,
    OP_ORU,
    OP_XORU,
    OP_NOTU,
    OP_SHRU,
    OP_SHLU,
    OP_CMPI,
    OP_CMPU,
    OP_CMPF,
    OP_GTI,
    OP_GEI,
    OP_GTU,
    OP_GEU,
    OP_GTF,
    OP_GEF,
    OP_LTI,
    OP_LEI,
    OP_LTU,
    OP_LEU,
    OP_LTF,
    OP_LEF,
} Opcode;

typedef union {
    alf_int  i;
    alf_uint u;
    alf_flt  f;
    char     c;
    String   *s;
} StkVal;

typedef alf_byte Instruction;

#define instdef(o) ((Instuction)0 | (o))
#define instext(i) (Opcode)((i) & 0x3f)

typedef struct {
    Address     entry;
    alf_byte    status;
    alf_uint    stksize;
    Address     sp, fp;
    const char  *program;
    StkVal      *stack;
} Alf_State;

#define alf_defer(a, s) { (a)->status = (s); goto defer; }

#endif // COMMON_H_
