#ifndef READER_H_
#define READER_H_

#include "arena.h"
#include "str.h"

typedef struct {
    size_t     size;
    Arena      allocs;
    const char *buf;
    const char *file;
} Reader;

#define reader_file(r, f)  ({ (r).file = (f); })
#define reader_slice(r) slice_parts((r)->buf, (r)->size);
#define reader_clean(r) arena_free(&(r).allocs);

extern int reader_read(Reader *r);

#endif // READER_H_
