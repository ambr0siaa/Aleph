#include "str.h"

long int slice_to_i64(Slice *s) {
    long int result = 0;
    size_t len = s->count;
    for (size_t i = 0; i < len; ++i)
        result = result * 10
               + s->items[i] - '0';
    return result;
}

int str_cmp(CString *s1, CString *s2) {
    if (s1->count == s2->count) {
        return strncmp(s1->items, s2->items, s1->count) == 0;
    } else {
        return 0;
    }
}

char interpolate_esc_char(char c) {
    switch (c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'e': return '\e';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '"': return '\"';
        case '\'': return '\'';
        case '\\': return '\\';
        default: return c;
    }
}

String *string(Arena *a, char *cstr, size_t len) {
    String *s = arena_alloc(a, sizeof(String));
    size_t i = 0;
    size_t j = i;
    for (; i < len; ++i) {
        if (cstr[i] == '\\') {
            arena_da_append_many(a, s, cstr + j, i - j);
            j = i;
            if (i + 1 < len) {
                arena_da_append(a, s, interpolate_esc_char(cstr[i + 1]));
                j += 2;
            } else {
                arena_da_append(a, s, '\\');
                goto defer;            
                break;
            }
        }
    }
    arena_da_append_many(a, s, cstr + j, i - j);
    defer: {
        arena_da_append(a, s, '\0');
        return s;
    }
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
