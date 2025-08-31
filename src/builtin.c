#include "core.h"

/*
 *  Note: this actually not all api for external function.
 *        Yeah in this language will be conection with C function
 *        and well all for this done already (actually not all but big part)
 */

void putstr(Alf_State *a) {
    String *s = fetch_data(a).s;
    char *p = s->items;
    while (*p) putchar(*p++);
}
