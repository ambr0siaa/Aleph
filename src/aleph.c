#include "parser.h"

static const char *shift_args(int *argc, char ***argv) {
    alf_assert(*argc > 0);
    const char *arg = **argv;
    *argc -= 1;
    *argv += 1;
    return arg;
}

static void alf_usage(const char *program) {
    panicf(UsgFmt,"%s [file.n] [options]\n"
           "options:\n"
           "...\n", program);
}

static inline Alf_State alf_state(const char *program) {
    Alf_State alf = {0};
    alf.codesize = alf.datasize = 1;
    alf.program = program;
    alf.status = ALF_STATUS_OK;
    alf.codeptr = alf.dataptr = 1;
    return alf;
}

// Coming soon...
static void parse_cmd_flag(Alf_State *alf, const char *flag) {
    (void)alf;
    (void)flag;
}

int main(int argc, char **argv) {
    Reader reader = {0};
    Alf_State alf = alf_state(shift_args(&argc, &argv));
    if (argc == 0) {
        panicf(alf.program, "Expected file path");
        alf_usage(alf.program);
        alf_defer(&alf, ALF_STATUS_FCK);
    }
    while (argc > 0) {
        const char *arg = shift_args(&argc, &argv);
        if (arg[0] == '-') parse_cmd_flag(&alf, arg);
        else reader_file(reader, arg);
    }
    reader_read(&reader);
    mainfunc(&alf, &reader);
    defer: {
        reader_clean(reader);
        return alf.status;
    }
}
