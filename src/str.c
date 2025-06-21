#include "str.h"

String_View string_view(const char *cstr) {
    return (String_View) {
        .count = strlen(cstr),
        .items = cstr
    };
}

String_Builder string_builder(char *cstr) {
    String_Builder sb = {.count = strlen(cstr), .capacity = SB_CAPACITY};
    while (sb.count > sb.capacity) {sb.capacity *= 2;}
    sb.items = malloc(sizeof(char)*sb.count + 1);
    memcpy(sb.items, cstr, sb.count);
    return sb;
}

String_Builder sb_from_cstr(char *cstr) {
    String_Builder sb = {.count = strlen(cstr), .capacity = SB_CAPACITY};
    while (sb.count > sb.capacity) {sb.capacity *= 2;}
    sb.items = cstr;
    return sb;
}

void sb_clean(String_Builder *sb) {
    free(sb->items);
    sb->capacity = 0;
    sb->count = 0;
}

void sb_join_null(String_Builder *sb) {
    sb->count++;
    if (sb->count > sb->capacity) {
        sb->capacity += 1;
        sb->items = realloc(sb->items, sizeof(char)*sb->count);
    }
    sb->items[sb->count - 1] = '\0';
}

String_View sb_to_sv(String_Builder sb) {
    return (String_View) {
        .count = sb.count,
        .items = sb.items
    };
}

void sv_trim_left(String_View *sv) {
    size_t i = 0;
    for (; i < sv->count && isspace(sv->items[i]); ++i);
    sv->count -= i;
    sv->items += i;
}

void sv_trim_right(String_View *sv) {
    size_t i = 0;
    for (; i < sv->count && isspace(sv->items[sv->count - i - 1]); ++i);
    sv->count -= i;
}

void sv_trim(String_View *sv) {
    sv_trim_right(sv);
    sv_trim_left(sv);
}
