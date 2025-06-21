#ifndef STR_H_
#define STR_H_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define Str_Fmt "%.*s"
#define Str_Args(s) (int)(s).count, (s).items

typedef struct {
    size_t count;
    const char *items;
} String_View;

String_View string_view(const char *cstr);
void sv_trim_left(String_View *sv);
void sv_trim_right(String_View *sv);
void sv_trim(String_View *sv);

typedef struct {
    size_t count;
    size_t capacity;
    char *items;
} String_Builder;

#define SB_CAPACITY 128

String_Builder string_builder(char *cstr);
String_Builder sb_from_cstr(char *cstr);
String_View sb_to_sv(String_Builder sb);
void sb_join_null(String_Builder *sb);
void sb_clean(String_Builder *sb);

#endif // STR_H_
