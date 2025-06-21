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

ALF_FUNC void alf_panic(Alf_State *alf, const char *log) {
    printf("error: %s\n", log);
    alf_usage(alf->program);
    alf->status = ALF_STATUS_FCK;
}

Alf_State alf_state(const char *program) {
    Alf_State alf = {0};
    alf.program = program;
    alf.status = ALF_STATUS_OK;
    return alf;
}

void alf_clean(Alf_State *alf) {
    // TODO: Not implemented
}

ALF_FUNC void parse_cmd_flag(Alf_State *alf, const char *flag) {
    // TODO: Not implemented
}

ALF_FUNC void parse_source_file(Alf_State *alf, const char *file_path) {
    // TODO: Not implemented
}

int main(int argc, char **argv) {
    Alf_State alf = alf_state(shift_args(&argc, &argv)); // Directly push program
    if (argc == 0) {
        alf_panic(&alf, "Expected file path");
        alf_defer_status(alf);
    }
    while (argc > 0) { // Checking cmd arguments
        const char *arg = shift_args(&argc, &argv);
        if (arg[0] == '-') parse_cmd_flag(&alf, arg);
        else parse_source_file(&alf, arg);
        alf_defer_status(alf);
    }
    alf_defer:
        alf_clean(&alf);
        return alf.status;
}
