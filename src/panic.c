#include "panic.h"

#define panic_common(f) do { \
    va_list args; \
    va_start(args, (f)); \
    vfprintf(stderr, (f), args); \
    fprintf(stderr, "\n"); \
    va_end(args); \
} while(0)

void panicf(const char *p, const char *fmt, ...) {
    fprintf(stderr, "%s: ", p);
    panic_common(fmt);
}

void panic(Location *l, const char *fmt, ...) {
    fprintf(stderr, "error at "Loc_Fmt, Loc_Args(*l));
    panic_common(fmt);
}
