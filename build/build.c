#define BIL_IMPLEMENTATION
#include "bil.h"

#define CC     "gcc"
#define OFLAGS "-O3", "-pipe", "-fPIE", "-march=native", "-mtune=native"
#define CFLAGS "-Wall", "-Wextra", "-flto"
#define DBFLAG "-ggdb"
#define TARGET "bin/aleph"

#define SRC "src/aleph.c", "src/str.c"
#define OBJ "src/aleph.o", "src/str.o"
#define HDR "src/common.h", "src/str.h"
#define DIR "bin"

static int g_flag, build_flag, o_flag;

BIL_FUNC void common_build_options(Bil_Cmd *cmd) {
    bil_cmd_append(cmd, CC);
    bil_cmd_append(cmd, CFLAGS);
    if (g_flag) bil_cmd_append(cmd, DBFLAG);
    else if (o_flag) bil_cmd_append(cmd, OFLAGS);
}

BIL_FUNC bool build_object_file(Bil_String_Builder src, Bil_String_Builder obj) {
    Bil_Cmd cmd = {0};
    common_build_options(&cmd);
    bil_cmd_append(&cmd, "-c", src.items);
    bil_cmd_append(&cmd, "-o", obj.items);
    if (!bil_cmd_run_sync(&cmd)) return false;
    return true;
}

BIL_FUNC void parse_cmd_args(int *argc, char ***argv) {
    while (*argc > 0) {
        const char *arg = bil_shift_args(argc, argv);
        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'b': build_flag = 1; break;
                case 'g': g_flag = 1;     break;
                case 'o': o_flag = 1;     break;
                default: {
                    bil_report(BIL_WARNING, "Unknown flag '%s'. Cannot parse it", arg);
                    break;
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    BIL_REBUILD(argc, argv, DIR);
    int status = BIL_EXIT_SUCCESS;
    parse_cmd_args(&argc, &argv);
    bil_workflow_begin(); {
        Bil_Cmd cmd = {0};
        Bil_Dep aleph = {0};
        Bil_Cstr_Array src = {0};
        bil_dep_init(&aleph, "bin/aleph.bil", SRC, HDR);
        bil_cstr_array_append(&src, SRC);
        bil_workflow_begin(); {
            for (size_t i = 0; i < src.count; ++i) { // Check that obj files are exist
                    Bil_String_Builder source = bil_sb_from_cstr(src.items[i]);
                    Bil_String_Builder object = bil_replace_file_extension(&source, "o");
                    bil_sb_join_nul(&source);
                    if (!bil_file_exist(object.items)) {
                        aleph.update = BIL_DEP_UPDATE_TRUE;
                        if (!build_object_file(source, object)) {
                            bil_defer_status(BIL_EXIT_FAILURE);
                        }
                    }
            }
        } bil_workflow_end(WORKFLOW_NO_TIME);
        if (bil_dep_ischange(&aleph)) {
            bil_workflow_begin(); {
                aleph.update = BIL_DEP_UPDATE_TRUE;
                for (size_t i = 0; i < aleph.changed.count; ++i) {
                        Bil_String_Builder source = bil_sb_from_cstr(aleph.changed.items[i]);
                        if (source.items[source.count - 1] != 'c') continue;
                        Bil_String_Builder object = bil_replace_file_extension(&source, "o");
                        bil_sb_join_nul(&source);
                        if (!build_object_file(source, object)) {
                            bil_defer_status(BIL_EXIT_FAILURE);
                        }
                }
            } bil_workflow_end(WORKFLOW_NO_TIME);
        }
        if (aleph.update || build_flag) {
            common_build_options(&cmd);
            bil_cmd_append(&cmd, OBJ);
            bil_cmd_append(&cmd, "-o", TARGET);
            if (!bil_cmd_run_sync(&cmd)) {
                bil_defer_status(BIL_EXIT_FAILURE);
            }
        } else {
            bil_report(BIL_INFO, "No changes");
        }
    } 
    defer: {
        bil_workflow_end();
        return status;
    }
}
