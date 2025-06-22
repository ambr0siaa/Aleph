#include <stdarg.h>
#include "common.h"


#define alf_defer_status(a) { if ((a).status == ALF_STATUS_FCK) goto alf_defer; }

ALF_FUNC const char *shift_args(int *argc, char ***argv) {
    assert(*argc > 0);
    const char *arg = **argv;
    *argc -= 1;
    *argv += 1;
    return arg;
}

ALF_FUNC void alf_usage(const char *program) {
    printf("usage: %s [file.n] [options]\n", program);
    printf("options:\n");
    printf("There is no way...\n");
}

ALF_FUNC void alf_panic(Alf_State *alf, const char *fmt, ...) {
    alf->status = ALF_STATUS_FCK;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: ", alf->program);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

Alf_State alf_state(const char *program) {
    Alf_State alf = {0};
    alf.program = program;
    alf.status = ALF_STATUS_OK;
    return alf;
}

void alf_clean(Alf_State *alf) {
    if (alf->source.items != NULL) {
        free((char*)alf->source.items);
        alf->source.count = 0;
    }
}

ALF_FUNC void parse_cmd_flag(Alf_State *alf, const char *flag) {
    (void)alf;
    (void)flag;
}

ALF_FUNC void parse_source_file(Alf_State *alf, const char *file_path) {
    alf->src_path = file_path;
    FILE *f = fopen(alf->src_path, "r");
    if (!f) {
        alf_panic(alf, "cannot open file:%s", alf->src_path);
        return;
    }
    if (fseek(f, 0, SEEK_END) < 0) {
        alf_panic(alf, "cannot read file size");
        return;
    }
    long int file_size = ftell(f);
    if (file_size < 0) {
        alf_panic(alf, "cannot read file size");
        return;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        alf_panic(alf, "cannot read file size");
        return;
    }
    alf->source.items = malloc(sizeof(char)*file_size);
    if (!alf->source.items) {
        alf_panic(alf, "cannot allocate buffer for source code");
        return;
    }
    memset((char*)alf->source.items, 0, file_size);
    size_t buf_size = fread((char*)alf->source.items, sizeof(char), file_size, f);
    if (buf_size != (size_t)file_size) {
        alf_panic(alf, "cannot read from file:%s", alf->src_path);
        return;
    }
    alf->source.count = buf_size;
    fclose(f);
}

int main(int argc, char **argv) {
    Alf_State alf = alf_state(shift_args(&argc, &argv)); // Directly push program
    if (argc == 0) {
        alf_panic(&alf, "Expected file path");
        alf_usage(alf.program);
        alf_defer_status(alf);
    }
    while (argc > 0) { // Checking cmd arguments
        const char *arg = shift_args(&argc, &argv);
        if (arg[0] == '-') parse_cmd_flag(&alf, arg);
        else parse_source_file(&alf, arg);
        alf_defer_status(alf);
    }
    printf("Has read:\n{'\n"Str_Fmt"'}\n", Str_Args(alf.source));
    alf_defer:
        alf_clean(&alf);
        return alf.status;
}
