#ifndef COMMON_H_
#define COMMON_H_

#include <assert.h>

#include "hashmap.h"
#include "arena.h"
#include "str.h"

typedef size_t        alf_uint;
typedef alf_uint      Address;
typedef unsigned char alf_byte;
typedef long int      alf_int;
typedef double        alf_flt;
typedef unsigned char alf_bool;

#define ALF_STATUS_OK  0
#define ALF_STATUS_FCK 1

typedef struct {
    Address     line;
    Address     offset;
    const char  *file;
} Location;

// 52
typedef enum {
    OP_PUSH,
    OP_POP,
    OP_STR,
    OP_PULL,
    OP_LOAD,
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
    OP_MODI,
    OP_ADDU,
    OP_SUBU,
    OP_DIVU,
    OP_MULU,
    OP_MODU,
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

#define instdef(o) ((Instuction)(o))
#define instext(i) (Opcode)((i) & 0x3f)

typedef struct {
    alf_byte    status;
    alf_uint    fnid;
    alf_uint    stksize;
    Address     fp, ip;
    Address     sp, lp, dp;
    alf_uint    datasize;
    alf_uint    codesize;
    alf_uint    codeptr;
    alf_uint    dataptr;
    const char  *program;
    Instruction **code;
    StkVal      **data;
    StkVal      *stack;
} Alf_State;

#define alf_defer(a, s) { (a)->status = (s); goto defer; }
#define alf_badcase(a)  if (a->status) goto defer; 
#define alf_assert(e)   assert(e)

#endif // COMMON_H_
