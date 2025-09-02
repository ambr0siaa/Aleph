#include "core.h"

/*
 *  Note: this actually not all api for external function.
 *        Yeah in this language will be conection with C function
 *        and well all for this done already (actually not all but big part)
 */

void putstr_(Alf_State *a) {
    String *s = fetch_data(a).s;
    char *p = s->items;
    while (*p) putchar(*p++);
}


void exit_(Alf_State *a) {
    alf_int code = fetch_data(a).i;
    a->status = code;
    a->bf = ALF_BREAK_PROGRAM;
}
