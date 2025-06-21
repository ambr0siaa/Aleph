#define BIL_IMPLEMENTATION
#include "bil.h"

#define CC      "gcc"
#define CFLAGS  "-Wall", "-Wextra", "-flto"
#define OFLAG   "-ggdb"

#define SRC "src/aleph.c", "src/str.c"
#define BIN "bin/aleph"
#define DIR "bin"

int main(int argc, char **argv) {
    BIL_REBUILD(argc, argv, DIR);
    int status = BIL_EXIT_SUCCESS;
    bil_workflow_begin(); {
        Bil_Cmd cmd = {0};
        bil_cmd_append(&cmd, CC);
        bil_cmd_append(&cmd, CFLAGS);
        bil_cmd_append(&cmd, SRC);
        bil_cmd_append(&cmd, "-o", BIN);
        if (!bil_cmd_run_sync(&cmd)) {
            status = BIL_EXIT_FAILURE;
        }
    } bil_workflow_end();
    return status;
}
