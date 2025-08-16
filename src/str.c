#include "str.h"

String *string(Arena *a, char *cstr, size_t len) {
    String *s = arena_alloc(a, sizeof(String));
    arena_da_append_many(a, s, cstr, len);
    arena_da_append(a, s, '\0');
    return s;
}

Slice slice(const char *cstr) {
    return (Slice) {
        .count = strlen(cstr),
        .items = cstr
    };
}

Slice slice_parts(const char *cstr, size_t len) {
    return (Slice) {
        .count = len,
        .items = cstr
    };
}

void slice_trim_left(Slice *slice) {
    size_t i = 0;
    for (; i <= slice->count
        && slice->items[i] != '\n'
        && slice->items[i] != '\r'
        && isspace(slice->items[i]);
        ++i);
    slice_shift(slice, i);
}

void slice_trim_right(Slice *slice) {
    size_t i = 0;
    for (; i <= slice->count 
        && slice->items[slice->count - i - 1] != '\n'
        && slice->items[slice->count - i - 1] != '\r'
        && isspace(slice->items[slice->count - i - 1]);
        ++i);
    slice->count -= i;
}
  
// Note: do not cut return or carriage return characters
void slice_trim(Slice *slice) {
    slice_trim_right(slice);
    slice_trim_left(slice);
}

void slice_shift(Slice *slice, size_t shift) {
    slice->items += shift;
    slice->count -= shift;
}
