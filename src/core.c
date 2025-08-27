#include <assert.h>
#include "core.h"

inline StkVal fetch_data(Alf_State *alf) {
    return alf->data[alf->lp][alf->dp++];
}

Opcode fetch_inst(Alf_State *alf) {
    return instext(alf->code[alf->lp][alf->ip++]);
}

static inline void stack_push(Arena *a, Alf_State *alf, StkVal v) {
    arena_da_append_parts(a, alf->stack, alf->sp, alf->stksize, v);
}

void putstr(Alf_State *a) {
    String *s = fetch_data(a).s;
    char *p = s->items;
    while (*p) putchar(*p++);
}

static inline int inst_exec(Arena *a, Alf_State *alf) {
    int status = 1;
    Opcode op = fetch_inst(alf);
    switch (op) {
        case OP_BLTIN: {
            Address addr = fetch_data(alf).u;
            if (addr == 0) putstr(alf);
            else {
                alf_panic("TODO", "Unkonwn built-in function");
                alf->status = ALF_STATUS_FCK;
            }
            break;
        }
        case OP_CALL: {
            Address addr = fetch_data(alf).u;
            alf_assert(addr < alf->codeptr);
            stack_push(a, alf, StkUint(alf->fp));
            stack_push(a, alf, StkUint(alf->ip));
            stack_push(a, alf, StkUint(alf->dp));
            stack_push(a, alf, StkUint(alf->lp));
            alf->ip = alf->dp = 0;
            alf->fp = alf->sp;
            alf->lp = addr;
            break;
        }
        case OP_RET: {
            alf_assert(alf->fp > 3);
            alf->lp = alf->stack[alf->fp - 1].u;
            alf->dp = alf->stack[alf->fp - 2].u;
            alf->ip = alf->stack[alf->fp - 3].u;
            alf->fp = alf->stack[alf->fp - 4].u;
            break;
        }
        case OP_HLT: {
            alf->status = ALF_STATUS_OK;
            status = 0;
            break;
        }
        default: {
            alf_panic(Err_Fmt, "Unknown instruction opcode `%u`", op);
            alf->status = ALF_STATUS_FCK;
            status = -1;
            break;
        }
    }
    return status;
}

static int inst_dump(Alf_State *a) {
    int status = 1;
    Opcode op = fetch_inst(a);
    switch (op) {
        case OP_PUSH: printf("op_push"); break;
        case OP_POP: printf("op_pop"); break;
        case OP_STR: printf("op_str"); break;
        case OP_PULL: printf("op_pull"); break;
        case OP_LOAD: printf("op_load"); break;
        case OP_BLTIN: printf("op_bltin"); break;
        case OP_JZ: printf("op_jz"); break;
        case OP_JNZ: printf("op_jnz"); break;
        case OP_CALL: printf("op_call"); break;
        case OP_RET: printf("op_ret"); break;
        case OP_HLT: printf("op_hlt"); status = 0; break;
        case OP_ADDI: printf("op_addi"); break;
        case OP_SUBI: printf("op_subi"); break;
        case OP_DIVI: printf("op_divi"); break;
        case OP_MULI: printf("op_muli"); break;
        case OP_MODI: printf("op_modi"); break;
        case OP_ADDU: printf("op_addu"); break;
        case OP_SUBU: printf("op_subu"); break;
        case OP_DIVU: printf("op_divu"); break;
        case OP_MULU: printf("op_mulu"); break;
        case OP_MODU: printf("op_modu"); break;
        case OP_ADDF: printf("op_addf"); break;
        case OP_SUBF: printf("op_subf"); break;
        case OP_DIVF: printf("op_divf"); break;
        case OP_MULF: printf("op_mulf"); break;
        case OP_ANDI: printf("op_andi"); break;
        case OP_ORI: printf("op_ori"); break;
        case OP_XORI: printf("op_xori"); break;
        case OP_NOTI: printf("op_noti"); break;
        case OP_SHRI: printf("op_shri"); break;
        case OP_SHLI: printf("op_shli"); break;
        case OP_ANDU: printf("op_andu"); break;
        case OP_ORU: printf("op_oru"); break;
        case OP_XORU: printf("op_xoru"); break;
        case OP_NOTU: printf("op_notu"); break;
        case OP_SHRU: printf("op_shru"); break;
        case OP_SHLU: printf("op_shlu"); break;
        case OP_CMPI: printf("op_cmpi"); break;
        case OP_CMPU: printf("op_cmpu"); break;
        case OP_CMPF: printf("op_cmpf"); break;
        case OP_GTI: printf("op_gti"); break;
        case OP_GEI: printf("op_gei"); break;
        case OP_GTU: printf("op_gtu"); break;
        case OP_GEU: printf("op_geu"); break;
        case OP_GTF: printf("op_gtf"); break;
        case OP_GEF: printf("op_gef"); break;
        case OP_LTI: printf("op_lti"); break;
        case OP_LEI: printf("op_lei"); break;
        case OP_LTU: printf("op_ltu"); break;
        case OP_LEU: printf("op_leu"); break;
        case OP_LTF: printf("op_ltf"); break;
        case OP_LEF: printf("op_lef"); break;
        default: printf("op unknown `%u`", op); status = -1; break;
    }
    return status;
}

void vm_run_program(Alf_State *a) {
    int status = 1;
    Arena mem = {0};
    a->ip = a->sp = a->lp = 0;
    a->fp = a->dp = 0;
    while (status > 0)
        status = inst_exec(&mem, a);
    arena_free(&mem);
}
