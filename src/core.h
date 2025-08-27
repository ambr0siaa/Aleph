#ifndef CORE_H_
#define CORE_H_

#include "common.h"
#include "parser.h"

typedef void (bltinfn)(Alf_State *a, Context *c);

extern void vm_run_program(Alf_State *a);
extern StkVal fetch_data(Alf_State *alf);

#endif // CORE_H_
