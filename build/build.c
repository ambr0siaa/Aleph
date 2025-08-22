#define BIL_IMPLEMENTATION
#include "bil.h"
#define BIL_STRIP_NAMESPACE

#define CC     "gcc"
#define OFLAGS "-O3", "-pipe", "-fPIE", "-march=native", "-mtune=native"
#define CFLAGS "-Wall", "-Wextra", "-flto"
#define DBFLAG "-ggdb"
#define TARGET "bin/aleph"

#define SRC "src/aleph.c", "src/str.c", "src/hashmap.c", "src/lexer.c", \
            "src/reader.c", "src/arena.c", "src/panic.c", "src/parser.c", "src/core.c"

#define OBJ "src/aleph.o", "src/str.o", "src/hashmap.o", "src/lexer.o", \
            "src/reader.o", "src/arena.o", "src/panic.o", "src/parser.o", "src/core.o"

#define HDR "src/common.h", "src/str.h", "src/hashmap.h", "src/lexer.h", \
            "src/reader.h", "src/arena.h",  "src/panic.h", "src/parser.h", "src/core.h"

#define DIR "bin"

static int g_flag, build_flag, o_flag, c_flag;

BILFN void common_build_options(Cmd *cmd) {
    cmd_append(cmd, CC);
    cmd_append(cmd, CFLAGS);
    if (g_flag) cmd_append(cmd, DBFLAG);
    else if (o_flag) cmd_append(cmd, OFLAGS);
}

BILFN bool build_object_file(String_Builder *src, String_Builder *obj) {
    Cmd cmd = {0};
    common_build_options(&cmd);
    cmd_append(&cmd, "-c", src->items);
    cmd_append(&cmd, "-o", obj->items);
    if (!cmd_run_sync(&cmd)) return false;
    return true;
}

BILFN void parse_cmd_args(int *argc, char ***argv) {
    while (*argc > 0) {
        const char *arg = shift_args(argc, argv);
        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'b': build_flag = 1; break;
                case 'g': g_flag = 1;     break;
                case 'o': o_flag = 1;     break;
                case 'c': {
                    c_flag = 1;
                    switch (arg[2]) {
                        case 'o': c_flag |= 2; break;
                        default: break;
                    }
                    break;
                }
                case 'h': {
                    report(BIL_INFO,
                           "Usage:\n"
                           "-b    build whole project without checking dependencies\n"
                           "-o    include optimisation flags to building process\n"
                           "-g    invlude debug information to buildgin process\n"
                           "-c    cleans all obj files and binarys\n"
                           "-h    print this usage");
                    break;
                }
                default: { 
                    report(BIL_WARNING, "Unknown flag '%s'. Cannot parse it", arg);
                    break;
                }
            }
        }
    }
}


BILFN int build_project(void) {
    report(BIL_INFO, "Start build whole project...");
    workflow_begin(); {
        Cmd cmd = {0};
        Cstr_Array src = {0};
        common_build_options(&cmd);
        da_append_items(&src, SRC);
        foreach(String_Builder, src, {
            if (!build_object_file(&source, &object))
                return EXIT_ERR;
        })
        cmd_append(&cmd, OBJ);
        cmd_append(&cmd, "-o", TARGET);
        if (!cmd_run_sync(&cmd))
            return EXIT_ERR;
    } workflow_end();
    return EXIT_OK;
}

BILFN int clean_command(void) {
    workflow_begin(); {
        Cmd cmd = {0};
        cmd_append(&cmd, "rm", "-rf", "test");
        if (!cmd_run_sync(&cmd)) return EXIT_FAILURE;
    } workflow_end(WORKFLOW_NO_TIME);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    SCRIPT_REBUILD(argc, argv, DIR);
    int status = EXIT_OK;
    parse_cmd_args(&argc, &argv);
    if (c_flag) return clean_command();
    if (build_flag) return build_project();
    workflow_begin(); {
        Cmd cmd = {0};
        Dep aleph = {0};
        Cstr_Array src = {0};
        dep_init(&aleph, "bin/aleph.bil", SRC, HDR);
        da_append_items(&src, SRC);
        workflow_begin(); {
            foreach(String_Builder, src, {
                if (!file_exist(object.items)) {
                    aleph.update = DEP_UPDATE_TRUE;
                    if (!build_object_file(&source, &object)) {
                        defer_status(EXIT_ERR);
                    }
                }
            })
        } workflow_end(WORKFLOW_NO_TIME);
        if (dep_ischange(&aleph)) {
            workflow_begin(); {
                aleph.update = DEP_UPDATE_TRUE;
                foreach(String_Builder, aleph.changed, {
                    if (source.items[source.count - 2] != 'c') continue;
                    if (!build_object_file(&source, &object)) {
                        defer_status(EXIT_ERR);
                    }
                })
            } workflow_end(WORKFLOW_NO_TIME);
        }
        if (aleph.update) {
            common_build_options(&cmd);
            cmd_append(&cmd, OBJ);
            cmd_append(&cmd, "-o", TARGET);
            if (!cmd_run_sync(&cmd)) {
                defer_status(EXIT_ERR);
            }
        } else {
            report(BIL_INFO, "No changes");
        }
    } 
    defer:
        workflow_end();
        return status;
}
