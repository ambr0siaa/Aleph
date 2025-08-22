#ifndef CORE_H_
#define CORE_H_

#include "common.h"
#include "parser.h"

typedef void (bltinfn)(Alf_State *a, Context *c);

ALF_API void vm_run_program(Alf_State *a);

#endif // CORE_H_
