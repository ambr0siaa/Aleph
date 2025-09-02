#ifndef ALEPH_H_
#define ALEPH_H_

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

typedef union {
    alf_int   i;
    alf_uint  u;
    alf_flt   f;
    char      c;
    String    *s;
    void      *p;
} StkVal;

typedef alf_byte Instruction;

#define instdef(o) ((Instuction)(o))
#define instext(i) (Opcode)((i) & 0x3f)

typedef struct Alf_State Alf_State;

typedef void (*builtinfn)(Alf_State *a);

struct Alf_State {
    int         status;
    alf_byte    cf, bf;
    alf_uint    fnid;
    alf_uint    stksize;
    Address     fp, ip;
    Address     sp, lp, dp;
    alf_uint    datasize;
    alf_uint    codesize;
    alf_uint    codeptr;
    alf_uint    dataptr;
    void*       builtins;
    const char  *program;
    Instruction **code;
    StkVal      **data;
    StkVal      *stack;
};

#define alf_defer(a, s) { (a)->status = (s); goto defer; }
#define alf_badcase(a)  if (a->status) goto defer; 
#define alf_assert(e)   assert(e)

#endif // ALEPH_H_
