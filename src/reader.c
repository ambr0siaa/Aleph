#include "reader.h"
#include "panic.h"

int reader_read(Reader *r) {
    FILE *f = fopen(r->file, "r");
    if (!f) {
        panicf(ErrFmt, "could not open file `%s`", r->file);
        return 0;
    }
    if (fseek(f, 0, SEEK_END) < 0) {
        panicf(ErrFmt, "could not read file size");
        return 0;
    }
    long int file_size = ftell(f);
    if (file_size < 0) {
        panicf(ErrFmt, "could not read file size");
        return 0;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        panicf(ErrFmt, "could not read file size");
        return 0;
    }
    r->buf = arena_alloc(&r->allocs, sizeof(char)*file_size);
    if (!r->buf) {
        panicf(ErrFmt, "could not allocate buffer for source code");
        return 0;
    }
    memset((char*)r->buf, 0, file_size);
    size_t buf_size = fread((char*)r->buf, sizeof(char), file_size, f);
    if (buf_size != (size_t)file_size) {
        panicf(ErrFmt, "could not read from file `%s`", r->file);
        return 0;
    }
    r->size = buf_size;
    fclose(f);
    return 1;
}
