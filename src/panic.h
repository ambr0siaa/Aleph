#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include "aleph.h"

#define UsgFmt  "usage"
#define ErrFmt  "error"
#define TodoFmt "todo"

#define Loc_Fmt     "%s:%zu:%zu: "
#define Loc_Args(l) (l).file, (l).line, (l).offset

extern void panicf(const char *p, const char *fmt, ...);
extern void panic(Location *l, const char *fmt, ...);

#endif // LOG_H_
