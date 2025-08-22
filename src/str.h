#ifndef STR_H_
#define STR_H_

#include "arena.h"
#include <string.h>
#include <ctype.h>

#define Str_Fmt "%.*s"
#define Str_Args(s) (int)(s).count, (s).items

typedef struct {
    size_t     count;
    const char *items;
} Slice;

typedef Slice CString; // Constant string

typedef struct {
    size_t count;
    size_t capacity;
    char   *items;
} String;             // Dynamic terminated string

extern Slice  slice(const char *cstr);
extern Slice  slice_parts(const char *cstr, size_t len);
extern void   slice_trim_left(Slice *s);
extern void   slice_trim_right(Slice *s);
extern void   slice_trim(Slice *s);
extern void   slice_shift(Slice *s, size_t shift);
extern int    str_cmp(CString *s1, CString *s2);
extern String *string(Arena *a, char *cstr, size_t len);

#endif // STR_H_
