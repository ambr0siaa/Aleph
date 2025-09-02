#ifndef CORE_H_
#define CORE_H_

#include "aleph.h"
#include "parser.h"

#define ALF_BREAK_PROGRAM    0
#define ALF_CONTINUE_PROGRAM 1


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

extern void vm_run_program(Alf_State *a);
extern StkVal fetch_data(Alf_State *alf);

#endif // CORE_H_
