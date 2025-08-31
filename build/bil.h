/*
*  This custom build system is like MakeFile, Cmake and etc
*  but implemented as single header style C library (stb-style).
*  Library can by used on Linux and Windows.
*  Project has inspired by `https://github.com/tsoding/nobuild`
* 
*  Copyright (c) 2024 Matthew Sokolkin <sokolkin227@gmail.com>
* 
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
* 
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
* 
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#ifndef BIL_H_
#define BIL_H_

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/wait.h>
#    include <sys/stat.h>
#    include <sys/time.h>
#    include <sys/types.h>
#endif

#define BILFN static

#define BIL_DA_INIT_CAPACITY 256

#define bil_da_size(da) (da)->capacity * sizeof(*(da)->items)

#define bil_assert  assert
#define bil_free    free
#define bil_exit    exit
#define bil_realloc bil_context_realloc

#define BIL_INFO_COLOR      "\x1b[1m"
#define BIL_ERROR_COLOR     "\x1b[38;5;9m"
#define BIL_NORMAL_COLOR    "\x1b[0m"
#define BIL_WARNING_COLOR   "\x1b[1m"

#define BIL_EXIT_OK  0
#define BIL_EXIT_ERR 1

#define BIL_DEP_UPDATE_TRUE  1
#define BIL_DEP_UPDATE_FALSE 0

#define BIL_DELETEME_FILE   "DELETEME"
#define BIL_CURRENT_DIR     "cur"

#define BIL_DOT_NEED    1
#define BIL_DOT_NOTNEED 0

#define BIL_ALIGNMENT sizeof(void*)
#define BIL_REGION_DEFAULT_CAPACITY (1024)
#define BIL_ASIZE_CMP(size) ((size) < BIL_REGION_DEFAULT_CAPACITY ? BIL_REGION_DEFAULT_CAPACITY : (size))

#define BIL_WORKFLOW_NO_TIME 1

#define Bil_Str_Args(sb) (int) (sb).count, (sb).items
#define Bil_Str_Fmt "%.*s"

#ifdef _WIN32
        typedef HANDLE Bil_Proc;
#       define BIL_INVALID_PROC INVALID_HANDLE_VALUE
#       define Bil_FileTime FILETIME
#       define bil_ft2li(t, dst) do { \
            (dst) = (long int)((uint64_t)((t).dwHighDateTime) << 32 | (t).dwLowDateTime); \
            (dst) = (dst) >= 0 ? (dst) : (-1)*(dst); \
        } while (0)
        int gettimeofday(struct timeval *tp);
#else
        typedef int Bil_Proc;
#       define Bil_FileTime time_t
#       define BIL_INVALID_PROC (-1)
#endif

typedef enum {
    BIL_INFO = 0,
    BIL_ERROR,
    BIL_WARNING
} bil_report_flags;

typedef struct {
    size_t capacity;
    size_t count;
    char   **items;
} Bil_Cstr_Array;

typedef struct {
    size_t capacity;
    size_t count;
    char   *items;
} Bil_String_Builder;

typedef struct {
    size_t count;
    char   *items;
} Bil_String_View;

typedef struct {
    size_t capacity;
    size_t count;
    char   **items;
} Bil_Cmd;

struct bil_dep_info {
    uint32_t     id;
    Bil_FileTime t;
    #ifdef _WIN32
        long int ltime;     /* load time from parser */
    #endif
};

/* TODO: binary search */
typedef struct {
    size_t              count;
    size_t              capacity;
    struct bil_dep_info *items;
} Bil_Deps_Info;

typedef struct {
    int            update;       // Did was any changes?
    Bil_Cstr_Array deps;         // Keep tracking dependences
    Bil_Cstr_Array changed;      // Already changed dependences after chicking in `bil_dep_ischange`
    const char     *output_file; // File where writes info about dependences
} Bil_Dep;

typedef struct Bil_Region Bil_Region;

struct Bil_Region {
    size_t     capacity;
    size_t     alloc_pos;
    Bil_Region *next;
    char       *data;
};

typedef struct WfContext WfContext;

struct WfContext {
    struct timeval begin;
    struct timeval end;
    Bil_Region     *r_head;
    Bil_Region     *r_tail;
    WfContext      *next;
};

typedef struct {
    WfContext *head, *tail;
} Bil_Workflow;

static int bil_alloc_flag;
static Bil_Workflow *workflow = NULL;

#ifndef BIL_REBUILD_COMMAND
#  define BIL_REBUILD_CFLAGS "-Wall", "-Wextra", "-flto", "-O3", "-fPIE", "-pipe"
#  if _WIN32
#    if defined(__GNUC__)
#       define BIL_REBUILD_COMMAND(source_path, output_path) "gcc", "-o", (output_path), (source_path), BIL_REBUILD_CFLAGS
#    elif defined(__clang__)
#       define BIL_REBUILD_COMMAND(source_path, output_path) "clang", "-o", (output_path), (source_path), BIL_REBUILD_CFLAGS
#    endif
#  else
#       define BIL_REBUILD_COMMAND(source_path, output_path) "gcc", "-o", (output_path), (source_path), BIL_REBUILD_CFLAGS
#  endif
#endif /* BIL_REBUILD_COMMAND */

#define BIL_SCRIPT_REBUILD(argc, argv, deleteme_dir) do {                                               \
        int status = -1;                                                                                \
        bil_workflow_begin(); {                                                                         \
            const char *output_file_path = (argv[0]);                                                   \
            const char *source_file_path = __FILE__;                                                    \
            bool rebuild_is_need = bil_check_for_rebuild(output_file_path, source_file_path);           \
            if (rebuild_is_need) {                                                                      \
                Bil_Cmd rebuild_cmd = {0};                                                              \
                const char *deleteme_path;                                                              \
                Bil_String_Builder deleteme_bil_sb_path = {0};                                          \
                if (strcmp(deleteme_dir, BIL_CURRENT_DIR)) {                                            \
                    deleteme_bil_sb_path = BIL_PATH(".", deleteme_dir, BIL_DELETEME_FILE);              \
                    deleteme_path = deleteme_bil_sb_path.items;                                         \
                } else {                                                                                \
                    deleteme_path = BIL_DELETEME_FILE;                                                  \
                }                                                                                       \
                if (!bil_rename_file(output_file_path, deleteme_path)) extra_exit();                    \
                bil_cmd_append(&rebuild_cmd, BIL_REBUILD_COMMAND(source_file_path, output_file_path));  \
                if (!bil_cmd_run_sync(&rebuild_cmd)) extra_exit();                                      \
                bil_report(BIL_INFO, "rebuild complete");                                               \
                Bil_Cmd cmd = {0};                                                                      \
                bil_da_append_many(&cmd, argv, argc);                                                   \
                status = !bil_cmd_run_sync(&cmd);                                                       \
            }                                                                                           \
        } bil_workflow_end(BIL_WORKFLOW_NO_TIME);                                                           \
        if (status != -1) bil_exit(status);                                                             \
    } while (0)

#define bil_da_append(da, new_item) do {                                                   \
        if ((da)->count + 1 >= (da)->capacity) {                                           \
            size_t old_size = bil_da_size((da));                                           \
            (da)->capacity = (da)->capacity > 0 ? (da)->capacity*2 : BIL_DA_INIT_CAPACITY; \
            (da)->items = bil_realloc((da)->items, old_size, bil_da_size((da)));           \
            bil_assert((da)->items != NULL);                                               \
        }                                                                                  \
        (da)->items[(da)->count++] = (new_item);                                           \
    } while (0)

#define bil_da_append_many(da, new_items, items_count) do {                                 \
        if ((da)->count + (items_count) >= (da)->capacity) {                                \
            size_t old_size = bil_da_size((da));                                            \
            if ((da)->capacity == 0) (da)->capacity = BIL_DA_INIT_CAPACITY;                 \
            while ((da)->count + (items_count) >= (da)->capacity) { (da)->capacity *= 2; }  \
            (da)->items = bil_realloc((da)->items, old_size, bil_da_size((da)));            \
            bil_assert((da)->items != NULL);                                                \
        }                                                                                   \
        memcpy((da)->items + (da)->count, (new_items), (items_count)*sizeof(*(da)->items)); \
        (da)->count += (items_count);                                                       \
    } while (0)


#define bil_da_append_da(dst, src) do {                                                          \
        if ((dst)->count + (src)->count >= (dst)->capacity) {                                    \
            size_t old_size = bil_da_size((dst));                                                \
            if ((dst)->capacity == 0) (dst)->capacity = BIL_DA_INIT_CAPACITY;                    \
            while ((dst)->count + (src)->count >= (dst)->capacity) { (dst)->capacity *= 2; }     \
            (dst)->items = bil_realloc((dst)->items, old_size, bil_da_size((dst)));              \
            bil_assert((dst)->items != NULL);                                                    \
        }                                                                                        \
        memcpy((dst)->items + (dst)->count, (src)->items, (src)->count * sizeof(*(src)->items)); \
        (dst)->count += (src)->count;                                                            \
    } while (0)

#define bil_da_clean(da) do {      \
        if ((da)->items != NULL    \
        &&  bil_alloc_flag) {      \
            bil_free((da)->items); \
            (da)->items = NULL;    \
            (da)->capacity = 0;    \
            (da)->count = 0;       \
        }                          \
    } while (0)

#define bil_da_append_items(da, ...) \
    bil_da_append_many((da), ((const char *[]){__VA_ARGS__}), \
                       sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char*))

#define bil_sb_join_nul(sb)  bil_da_append_many(sb, "", 1)
#define bil_sb_join(sb,...)  bil_sb_join_many((sb), __VA_ARGS__, NULL)

#define bil_cmd_append(cmd, ...) \
    bil_da_append_many(cmd, ((const char *[]){__VA_ARGS__}), \
                       sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char*))

#define bil_cmd_clean(cmd) bil_da_clean(cmd)
#define bil_cmd_reset(cmd) do (cmd).count = 0; while(0)
#define bil_cmd_append_cmd(dst, src) bil_da_append_da(dst, src)

#define bil_check_deps(...) \
    bil_deps_ischange(((const Bil_Dep[]){__VA_ARGS__}), \
                       sizeof((const Bil_Dep[]){__VA_ARGS__}) / sizeof(const Bil_Dep))

#define bil_dep_init(dep, output_file_path, ...) do {                                   \
        (dep)->update = BIL_DEP_UPDATE_FALSE;                                           \
        (dep)->output_file = output_file_path;                                          \
        bil_da_append_many(&(dep)->deps, ((const char*[]){__VA_ARGS__}),          \
                                 sizeof((const char*[]){__VA_ARGS__})/sizeof(char*));   \
    } while (0)

#define BIL_PATH(file, ...) bil_mk_path(file, __VA_ARGS__, NULL)
#define bil_defer_status(s) do { status = (s); goto defer; } while(0)

#define bil_workflow_end(...) \
    workflow_end_(((const int[]){__VA_ARGS__}), sizeof((const int[]){__VA_ARGS__}) / sizeof(const int));

#define foreach(type, src, body) \
    for (size_t i = 0; i < (src).count; ++i) { \
        type source = bil_sb_from_cstr((src).items[i]); \
        type object = bil_replace_file_extension(&source, "o"); \
        bil_sb_join_nul(&source); \
        body  \
    }

void bil_report(bil_report_flags flag, char *fmt, ...);

void bil_sb_join_many(Bil_String_Builder *sb, ...);
void bil_sb_join_cstr(Bil_String_Builder *sb, const char *cstr);
Bil_String_Builder bil_sb_from_cstr(char *cstr);

Bil_String_View bil_sv_from_cstr(char *cstr);
Bil_String_View bil_sv_cut_value(Bil_String_View *sv);
Bil_String_View bil_sv_chop_by_space(Bil_String_View *sv);
void bil_sv_cut_space_left(Bil_String_View *sv);
int bil_char_in_sv(Bil_String_View sv, char c);
int bil_sv_cmp(Bil_String_View s1, Bil_String_View s2);
uint32_t bil_sv_to_u32(Bil_String_View sv);
char *bil_sv_to_cstr(Bil_String_View sv);

Bil_String_Builder bil_cmd_create(Bil_Cmd *cmd);
Bil_Proc bil_cmd_run_async(Bil_Cmd *cmd);
bool bil_cmd_run_sync(Bil_Cmd *cmd);
bool bil_proc_await(Bil_Proc proc);

bool bil_dep_ischange(Bil_Dep *dep); /* the main workhorse function for dependencies */
int *bil_deps_ischange(const Bil_Dep *deps, size_t count); /* checks many dependeces */
Bil_Deps_Info bil_parse_dep(char *src);
bool bil_dep_write(const char *path, Bil_String_Builder *dep);
uint32_t bil_file_id(char *file_path);
Bil_FileTime bil_file_last_update(const char *path);
Bil_String_Builder bil_mk_dependence_file(Bil_Cstr_Array deps);
Bil_String_Builder bil_mk_dependence_from_info(Bil_Deps_Info *info);
void bil_change_dep_info(Bil_Deps_Info *info, struct bil_dep_info new_info);
bool bil_deps_info_search(Bil_Deps_Info *info, uint32_t id, struct bil_dep_info *target); 

bool bil_check_for_rebuild(const char *output_file_path, const char *source_file_path);
char *bil_shift_args(int *argc, char ***argv);

void bil_makedir(const char *name);
bool bil_dir_exist(const char *dir_path);
Bil_String_Builder bil_mk_path(char *file, ...);
void extra_exit();

bool bil_file_exist(const char *file_path);
bool bil_delete_file(const char *file_path);
bool bil_read_file(const char *file_path, char **dst);
bool bil_read_entire_file(const char *file_path, Bil_String_View *dst);
bool bil_rename_file(const char *file_path, const char *new_path);
void bil_cut_file_extension(Bil_String_Builder *file, bool dot);
Bil_String_Builder bil_replace_file_extension(Bil_String_Builder *file, const char *ext);

Bil_Region *bil_region_create(size_t region_size);
void *bil_context_alloc(size_t size);
void *bil_context_realloc(void *ptr, size_t old_size, size_t new_size);

double workflow_time_taken(struct timeval end, struct timeval begin);
void bil_workflow_begin();
void workflow_end_(const int *args, int argc);

#endif /* BIL_H_ */

#ifdef BIL_IMPLEMENTATION

void bil_cut_file_extension(Bil_String_Builder *file, bool dot)
{
    size_t i = 0;
    while (i < file->count && file->items[file->count - i - 1] != '.') ++i;
    if (!dot) --i;
    file->count -= i;
}

Bil_String_Builder bil_replace_file_extension(Bil_String_Builder *file, const char *ext)
{
    Bil_String_Builder sb = bil_sb_from_cstr(file->items);
    bil_cut_file_extension(&sb, BIL_DOT_NEED);
    bil_sb_join_cstr(&sb, ext);
    bil_sb_join_nul(&sb);
    return sb;
}

void extra_exit()
{
    if (workflow != NULL) {
        WfContext *wfc = workflow->head;
        while (wfc != NULL) {
            Bil_Region *r = wfc->r_head;
            while (r != NULL) {
                Bil_Region *next = r->next;
                free(r->data);
                free(r);
                r = next;
            }
            WfContext *next = wfc->next;
            free(wfc);
            wfc = next;
        } 
    } else
        bil_report(BIL_WARNING, "Some allocations may not cleaned");
    bil_exit(BIL_EXIT_ERR);
}

#ifdef _WIN32
/* Stolen from https://stackoverflow.com/questions/10905892/equivalent-of-gettimeofday-for-windows */
int gettimeofday(struct timeval *tp)
{
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);
    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime( &system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;
    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
#endif

Bil_Region *bil_region_create(size_t region_size)
{
    Bil_Region *r = malloc(sizeof(Bil_Region));
    r->data = malloc(region_size);
    r->capacity = region_size;
    r->alloc_pos = 0;
    r->next = NULL;
    return r;
}

void *bil_context_alloc(size_t size)
{
    if (workflow == NULL) {
        bil_alloc_flag = 1;
        bil_report(BIL_WARNING, "Default malloc is used. Keep memory clean!");
        return malloc(size);
    }
    bil_alloc_flag = 0;
    WfContext *actual = workflow->tail;
    Bil_Region *cur = actual->r_head;
    size_t align_size = (size + (BIL_ALIGNMENT - 1)) & ~(BIL_ALIGNMENT - 1);
    for(;;) {
        if (cur == NULL) {
            Bil_Region* r = bil_region_create((size) < BIL_REGION_DEFAULT_CAPACITY ?
                                              BIL_REGION_DEFAULT_CAPACITY : (size));
            actual->r_tail->next = r;
            actual->r_tail = r;
            cur = actual->r_tail;
        }
        if (cur->alloc_pos + align_size <= cur->capacity) {
            char *ptr = (char*)(cur->data + cur->alloc_pos);
            memset(ptr, 0, align_size);
            cur->alloc_pos += align_size;
            return ptr;
        } else {
            cur = cur->next;
            continue;
        }
    }
}

void *bil_context_realloc(void *ptr, size_t old_size, size_t new_size)
{
    if (new_size > old_size) {
        void *new_ptr = bil_context_alloc(new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

void bil_workflow_begin()
{
    WfContext *wfc = malloc(sizeof(WfContext));
    wfc->end = (struct timeval) {0};
#ifdef _WIN32
    gettimeofday(&wfc->begin);
#else
    gettimeofday(&wfc->begin, NULL);
#endif
    Bil_Region *r = bil_region_create(BIL_REGION_DEFAULT_CAPACITY);
    wfc->r_head = r;
    wfc->r_tail = r;
    wfc->next = NULL;
    if (workflow == NULL) {
        workflow = malloc(sizeof(Bil_Workflow));
        workflow->head = wfc;
        workflow->tail = wfc;
    } else {
        workflow->tail->next = wfc;
        workflow->tail = wfc;
    }
}

double workflow_time_taken(struct timeval end, struct timeval begin)
{
    double time_taken = (end.tv_sec - begin.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - begin.tv_usec)) * 1e-6;
    return time_taken;
}

void workflow_end_(const int *args, int argc)
{
    if (workflow == NULL) {
        bil_report(BIL_WARNING, "Cannot clear workflow that not exist. All workflows have already done or haven't begun\n");
        return;
    }
    WfContext *wfc = workflow->tail;
    if (argc < 1) {
    workflow_time:
    #ifdef _WIN32
        gettimeofday(&wfc->end);
    #else
        gettimeofday(&wfc->end, NULL);
    #endif
        double time_taken = workflow_time_taken(wfc->end, wfc->begin);
        bil_report(BIL_INFO, "Workflow has taken %lf sec", time_taken);
    } else if (argc > 0 && args[0] != BIL_WORKFLOW_NO_TIME) {
        goto workflow_time;
    }
    Bil_Region *r = wfc->r_head;
    while(r != NULL) {
        Bil_Region *r_next = r->next;
        free(r->data);
        free(r);
        r = r_next;
    }
    WfContext *cur = workflow->head;
    if (cur == wfc) {
        free(wfc);
        free(workflow);
        workflow = NULL;
        return;
    }
    while (cur->next != wfc) cur = cur->next;
    free(wfc);
    cur->next = NULL;
    workflow->tail = cur;
}

uint32_t bil_sv_to_u32(Bil_String_View sv)
{
    uint32_t result = 0;
    for (size_t i = 0; i < sv.count && isdigit(sv.items[i]); ++i)
        result = result * 10 + sv.items[i] - '0';
    return result;
}

char *bil_sv_to_cstr(Bil_String_View sv)
{
    char *cstr = bil_context_alloc(sv.count + 1);
    memcpy(cstr, sv.items, sv.count);
    cstr[sv.count] = '\0';
    return cstr;
}

Bil_String_View bil_sv_from_cstr(char *cstr)
{
    return (Bil_String_View) {
        .count = strlen(cstr),
        .items = cstr
    };
}

Bil_String_View bil_sv_cut_value(Bil_String_View *sv)
{
    Bil_String_View result;
    size_t i = 0;
    while (i < sv->count && (isdigit(sv->items[i]) || sv->items[i] == '.')) ++i;
    result.items = sv->items;
    result.count = i;
    sv->count -= i;
    sv->items += i;
    return result;
}

int bil_char_in_sv(Bil_String_View sv, char c)
{
    int result = 0;
    for (size_t i = 0; i < sv.count; ++i) {
        if (sv.items[i] == c) {
            result = 1;
            break;
        }
    }

    return result;
}

void bil_sv_cut_space_left(Bil_String_View *sv)
{
    size_t i = 0;
    while (i < sv->count && isspace(sv->items[i])) ++i;
    sv->count -= i;
    sv->items += i;
}

int bil_sv_cmp(Bil_String_View s1, Bil_String_View s2)
{
    if (s1.count == s2.count) {
        return memcmp(s1.items, s2.items, s1.count) == 0;
    } else {
        return 0;
    }
}

Bil_String_View bil_sv_chop_by_space(Bil_String_View *sv)
{
    Bil_String_View result = {0};
    size_t i = 0;
    while (i < sv->count && !isspace(sv->items[i])) ++i;
    result.items = sv->items;
    result.count = i;
    sv->count -= i;
    sv->items += i;
    return result;
}

void bil_sv_cut_left(Bil_String_View *sv, int shift)
{
    sv->items += shift;
    sv->count -= shift;
}

Bil_Deps_Info bil_parse_dep(char *src)
{
    Bil_Deps_Info info = {0};
    Bil_String_View source = bil_sv_from_cstr(src);
    while (source.count > 0 && source.items[0] != '\0') {
        struct bil_dep_info i = {0};
        if (isdigit(source.items[0])) {
            i.id = bil_sv_to_u32(bil_sv_cut_value(&source));
            bil_sv_cut_left(&source, 1);
            if (source.items[0] == '-')
                bil_sv_cut_left(&source, 1);
            #ifdef _WIN32
            i.ltime = bil_sv_to_u32(bil_sv_cut_value(&source));
            #else
            i.t = bil_sv_to_u32(bil_sv_cut_value(&source));
            #endif
            bil_sv_cut_left(&source, 1);
        } else if (source.items[0] == '-') {
            bil_sv_cut_left(&source, 1);
        } else {
            bil_report(BIL_ERROR, "unknown character `%c`", source.items[0]);
            extra_exit();
        }
        bil_da_append(&info, i);
    }
    return info;
}

void bil_makedir(const char *dir_path)
{
    bil_report(BIL_INFO, "Make directory %s", dir_path);
#ifdef _WIN32
    if (CreateDirectoryA(dir_path, NULL) == 0) {
        bil_report(BIL_ERROR, "cannot create directory `%s`: %lu",
                dir_path, GetLastError());
        extra_exit();
    }
#else
    if (mkdir(dir_path, 0777) < 0) {
        bil_report(BIL_ERROR, "cannot create directory `%s`: %s\n",
                dir_path, strerror(errno));
        extra_exit();
    }
#endif
}

bool bil_dir_exist(const char *dir_path)
{
    bool result = true;
    DIR *dir = opendir(dir_path);
    if (dir) {
        result = true;
    } else if (ENOENT == errno) {
        result = false;
    } else {
        bil_report(BIL_ERROR, "file `%s` not a directory\n", dir_path);
        result = false;
    }
    closedir(dir);
    return result;
}

/* TODO: not implemented */
void bil_wildcard()
{
    DIR *d;
    struct dirent *dir;
    
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
}

bool bil_read_file(const char *file_path, char **dst)
{
    bool status = true;
    FILE *f = fopen(file_path, "r");
    if (!f) bil_defer_status(false);
    if (fseek(f, 0, SEEK_END) < 0)
        bil_defer_status(false); 
    long int file_size = ftell(f);
    if (file_size < 0)
        bil_defer_status(false);
    if (fseek(f, 0, SEEK_SET) < 0)
        bil_defer_status(false);
    char *buf = bil_context_alloc(file_size + 1);
    if (!buf) bil_defer_status(false);
    size_t buf_len = fread(buf, 1, file_size, f);
    buf[buf_len] = '\0';
    if (ferror(f))
        bil_defer_status(false);
    *dst = buf;
defer:
    fclose(f);
    if (status != true)
        bil_report(BIL_ERROR, "cannot read from `%s`: %s",
                file_path, strerror(errno));
    return status;
}

bool bil_read_entire_file(const char *file_path, Bil_String_View *dst)
{
    char *content;
    if (!bil_read_file(file_path, &content))
        return false;
    *dst = bil_sv_from_cstr(content);
    return true;
}

Bil_FileTime bil_file_last_update(const char *path)
{
    Bil_FileTime result;
    if (!bil_file_exist(path)) {
        bil_report(BIL_ERROR, "cannot find dependence by path `%s`", path);
        extra_exit();
    }
#ifdef _WIN32
    BOOL Proc;
    HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (file == BIL_INVALID_PROC) {
        bil_report(BIL_ERROR, "cannot open `%s`: %lu", path, GetLastError());
        extra_exit();
    }
    Bil_FileTime file_time;
    Proc = GetFileTime(file, NULL, NULL, &file_time);
    CloseHandle(file);
    if (!Proc) {
        bil_report(BIL_ERROR, "cannot get `%s` time: %lu", path, GetLastError());
        extra_exit();
    }
    result = file_time;
#else
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        bil_report(BIL_ERROR, "cannot get info about `%s`: %s", strerror(errno));
        extra_exit();
    }
    result = statbuf.st_ctime;
#endif
    return result;
}

bool bil_dep_write(const char *path, Bil_String_Builder *dep)
{
    bool status = true;
    FILE *f = fopen(path, "w");
    if (!f) {
        bil_report(BIL_ERROR, "cannot open file by path `%s`", path);
        bil_defer_status(false);
    }
    fwrite(dep->items, sizeof(dep->items[0]), sizeof(dep->items[0]) * dep->count, f);
    if (ferror(f)) {
        bil_report(BIL_ERROR, "cannot write to `%s` file", path);
        bil_defer_status(false);
    }
defer:
    fclose(f);
    return status;
}

/* Check summ CRC32B */
uint32_t bil_file_id(char *file_path)
{
    size_t i = 0;
    uint32_t id = 0xFFFFFFFF, mask;
    while (file_path[i]) {
        id ^= (uint32_t)(file_path[i]);
        for (int j = 7; j >= 0; --j) {
            mask = -(id & 1);
            id = (id >> 1) ^ (0xEDB88320 & mask);
        }
        ++i;
    }
    return ~id;
}

Bil_String_Builder bil_mk_dependence_file(Bil_Cstr_Array deps)
{
    Bil_String_Builder sb = {0};
    for (size_t i = 0; i < deps.count; ++i) {
        char buf[256];
        char *file_path = deps.items[i];
        Bil_FileTime t = bil_file_last_update(file_path);
        long int time;
        #ifdef _WIN32
        bil_ft2li(t, time);
        #else
        time = t;
        #endif
        uint32_t id = bil_file_id(file_path);
        snprintf(buf, sizeof(buf), "%u %li\n", id, time);
        bil_sb_join_cstr(&sb, buf);
    }
    bil_sb_join_nul(&sb);
    return sb;
}

bool bil_deps_info_search(Bil_Deps_Info *info, uint32_t id, struct bil_dep_info *target)
{
    bool result = false;
    for (size_t i = 0; i < info->count; ++i) {
        if (info->items[i].id == id) {
            *target = info->items[i];
            result = true;
            break;
        }
    }
    return result;
}

void bil_change_dep_info(Bil_Deps_Info *info, struct bil_dep_info new_info)
{
    for (size_t i = 0; i < info->count; ++i) {
        if (new_info.id == info->items[i].id) {
            info->items[i].t = new_info.t;
            #ifdef _WIN32
            bil_ft2li(new_info.t, info->items[i].ltime);
            #endif
            break;
        }
    }
    info = info;
}

Bil_String_Builder bil_mk_dependence_from_info(Bil_Deps_Info *info)
{
    Bil_String_Builder sb = {0};
    for (size_t i = 0; i < info->count; ++i) {
        char buf[256];
        struct bil_dep_info new = info->items[i];
        long int time;
        #ifdef _WIN32
        time = new.ltime;
        time = time >= 0 ? time : (-1) * time;
        #else
        time = new.t;
        #endif
        snprintf(buf, sizeof(buf), "%u %li\n", info->items[i].id, time);
        bil_sb_join_cstr(&sb, buf);
    }
    bil_sb_join_nul(&sb);
    return sb;
}

bool bil_dep_ischange(Bil_Dep *dep)
{
    bool result = false;
    const char *update = NULL;
    char *dependence;
    if (!bil_file_exist(dep->output_file)) {
    dep_write:
        Bil_String_Builder out = bil_mk_dependence_file(dep->deps);
        bil_dep_write(dep->output_file, &out);
        if (update) {
            bil_report(BIL_INFO, "Updated dependence output `%s`: new file %s%s%s",
                       dep->output_file, BIL_INFO_COLOR, dependence, BIL_NORMAL_COLOR);
        } else {
            bil_report(BIL_INFO, "Created dependece output `%s`", dep->output_file);
        }
        dep->update = BIL_DEP_UPDATE_TRUE;
        bil_da_clean(&out);
        return true;
    }
    char *buf;
    if (!bil_read_file(dep->output_file, &buf)) return false;
    Bil_Deps_Info info = bil_parse_dep(buf);
    for (size_t i = 0; i < dep->deps.count; ++i) {
        bool changed = false;
        dependence = dep->deps.items[i];
        struct bil_dep_info dependence_info = { 
            .id = bil_file_id(dependence),
            .t  = bil_file_last_update(dependence)
        };
        struct bil_dep_info target = {0};
        bool found = bil_deps_info_search(&info, dependence_info.id, &target);
        if (!found) {
            dep->update = BIL_DEP_UPDATE_TRUE;
            update = dependence;
            goto dep_write;
        }
        #ifdef _WIN32
            long int dependece_time;
            bil_ft2li(dependence_info.t, dependece_time);
            if (target.ltime != dependece_time)
        #else 
            if (dependence_info.t != target.t)
        #endif
        {
            result = true;
            changed = true;
            dep->update = BIL_DEP_UPDATE_TRUE;
        }
        if (changed == true) {
            bil_da_append_items(&dep->changed, dependence);
            bil_change_dep_info(&info, dependence_info);
            bil_report(BIL_INFO, ": %sFILE%s %s : dependence %s%s%s has changed",
                       BIL_INFO_COLOR, BIL_NORMAL_COLOR, dep->output_file,
                       BIL_INFO_COLOR, dependence, BIL_NORMAL_COLOR);
        }
    }
    if (result == true) {
        Bil_String_Builder out = bil_mk_dependence_from_info(&info);
        bil_dep_write(dep->output_file, &out);
        bil_da_clean(&out);
    }
    if (bil_alloc_flag) free(buf);
    bil_da_clean(&info);
    return result;
}

int *bil_deps_ischange(const Bil_Dep *deps, size_t count)
{
    int *changes = bil_context_alloc(sizeof(int)*count);
    for (size_t i = 0; i < count; ++i)
        changes[i] = bil_dep_ischange((Bil_Dep*)(&deps[i]));
    return changes;
}

Bil_String_Builder bil_mk_path(char *file, ...)
{
    Bil_String_Builder sb = {0};
    bil_sb_join_cstr(&sb, file);
    bil_sb_join_cstr(&sb, "/");
    va_list args;
    va_start(args, file);
    char *arg = va_arg(args, char*);
    while (1) {
        bil_sb_join_cstr(&sb, arg);
        arg = va_arg(args, char*);
        if (arg == NULL) break;
        bil_sb_join_cstr(&sb, "/");
    }
    va_end(args);
    return sb;
}

void bil_report(bil_report_flags flag, char *fmt, ...)
{
    fprintf(stderr, ":: ");
    switch (flag) {
    case BIL_INFO:
        fprintf(stderr, "%sINFO%s ", 
                BIL_INFO_COLOR, BIL_NORMAL_COLOR);
        break;
    case BIL_ERROR:
        fprintf(stderr, "%sERROR%s ", 
                BIL_ERROR_COLOR, BIL_NORMAL_COLOR);
        break;
    case BIL_WARNING:
        fprintf(stderr, "%sWARNING%s ",
                BIL_WARNING_COLOR, BIL_NORMAL_COLOR);
        break;
    default:
        bil_assert(0 && "Unknown flag");
        break;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    printf("\n");
}

Bil_String_Builder bil_sb_from_cstr(char *cstr)
{
    size_t len = strlen(cstr);
    size_t capacity = BIL_DA_INIT_CAPACITY;
    if (len > capacity) {
        do { capacity *= 2; } while (len > capacity);
    }
    char *buf = bil_context_alloc(capacity * sizeof(char));
    memcpy(buf, cstr, len);
    return (Bil_String_Builder) {
        .capacity = capacity,
        .count = len,
        .items = buf
    };
}

void bil_sb_join_cstr(Bil_String_Builder *sb, const char *cstr)
{
    size_t len = strlen(cstr);
    bil_da_append_many(sb, cstr, len);
}

void bil_sb_join_many(Bil_String_Builder *sb, ...)
{
    va_list args;
    va_start(args, sb);
    char *arg = va_arg(args, char*);
    while (arg != NULL) {
        bil_sb_join_cstr(sb, arg);
        arg = va_arg(args, char*);
    }
    va_end(args);
}

Bil_String_Builder bil_cmd_create(Bil_Cmd *cmd)
{
    Bil_String_Builder sb = {0};
    for (size_t i = 0; i < cmd->count; ++i) {
        bil_sb_join(&sb, cmd->items[i]);
        if (i + 1 != cmd->count) bil_sb_join(&sb, " ");
    }
    return sb;
}

Bil_Proc bil_cmd_run_async(Bil_Cmd *cmd)
{
    if (cmd->count < 1) {
        bil_report(BIL_ERROR, "Could not execute empty command\n");
        return BIL_INVALID_PROC;
    }
    Bil_String_Builder command = bil_cmd_create(cmd);
    bil_sb_join_nul(&command);
    bil_report(BIL_INFO, ": %sCMD%s [%s]",
            BIL_INFO_COLOR, BIL_NORMAL_COLOR,command.items);
#ifdef _WIN32
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    BOOL proc = CreateProcessA(NULL, command.items, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!proc) {
        bil_report(BIL_ERROR, "Could not create child process: %lu", GetLastError());
        return BIL_INVALID_PROC;
    }
    CloseHandle(pi.hThread);
    bil_da_clean(&command);
    return pi.hProcess;
#else
    bil_da_clean(&command);
    pid_t cpid = fork();
    if (cpid < 0) {
        bil_report(BIL_ERROR, "Could not fork child process: %s", strerror(errno));
        return BIL_INVALID_PROC;
    }
    if (cpid == 0) {
        Bil_Cmd cmd_null = {0};
        bil_cmd_append_cmd(&cmd_null, cmd);
        bil_cmd_append(&cmd_null, NULL);
        if (execvp(cmd->items[0], (char * const*) cmd_null.items) < 0) {
            bil_report(BIL_ERROR, "Could not execute child process: %s", strerror(errno));
        }
        bil_cmd_clean(&cmd_null);
    }
    return cpid;
#endif
}

char *bil_shift_args(int *argc, char ***argv)
{
    bil_assert(*argc >= 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

bool bil_proc_await(Bil_Proc proc)
{
#ifdef _WIN32
    DWORD result = WaitForSingleObject(proc, INFINITE);
    if (result == WAIT_FAILED) {
        bil_report(BIL_ERROR, "Could not wait on child process: %lu", GetLastError());
        return false;
    }
    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        bil_report(BIL_ERROR, "Could not get process exit code: %lu", GetLastError());
        return false;
    }
    if (exit_status != 0) {
        bil_report(BIL_ERROR, "Command exited with exit code %lu", exit_status);
        return false;
    }
    CloseHandle(proc);
#else
    for(;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            bil_report(BIL_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }
        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                bil_report(BIL_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }
            break;
        }
        if (WIFSIGNALED(wstatus)) {
            bil_report(BIL_ERROR, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }
#endif /* _WIN32 */
    return true;
}

bool bil_cmd_run_sync(Bil_Cmd *cmd)
{
    Bil_Proc proc = bil_cmd_run_async(cmd);
    if (proc == BIL_INVALID_PROC) return false;
    return bil_proc_await(proc);
}

bool bil_check_for_rebuild(const char *output_file_path, const char *source_file_path)
{
    Bil_FileTime src_file_time = bil_file_last_update(source_file_path);
    Bil_FileTime output_file_time = bil_file_last_update(output_file_path);
    #ifdef _WIN32
    if (CompareFileTime(&src_file_time, &output_file_time) == 1) return true;
    #else
    if (src_file_time > output_file_time) return true;
    #endif
    return false;
}

bool bil_rename_file(const char *file_name, const char *new_name)
{
    bil_report(BIL_INFO, "renaming %s -> %s", file_name, new_name);
#ifdef _WIN32
    if (!MoveFileEx(file_name, new_name, MOVEFILE_REPLACE_EXISTING)) {
        bil_report(BIL_ERROR, "could not rename %s to %s: %lu", file_name, new_name, GetLastError());
        return false;
    }
#else
    if (rename(file_name, new_name) < 0) {
        bil_report(BIL_ERROR, "could not rename %s to %s: %s", file_name, new_name, strerror(errno));
        return false;
    }
#endif
    return true;
}

bool bil_delete_file(const char *file_path)
{
#ifdef _WIN32
    BOOL fSuccess = DeleteFile(file_path);
    if (!fSuccess) {
        bil_report(BIL_ERROR, "Cannot delete file by path `%s`", file_path);
        return false;
    }
#else
    if (remove(file_path) != 0) {
        bil_report(BIL_ERROR, "Cannot delete file by path `%s`", file_path);
        return false;
    }
#endif
    bil_report(BIL_INFO, "file %s%s%s was deleted",
               BIL_INFO_COLOR, file_path, BIL_NORMAL_COLOR);
    return true;
}

bool bil_file_exist(const char *file_path)
{
#ifdef _WIN32
    DWORD ret = GetFileAttributes(file_path);
    return ((ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY));
#else
    if (access(file_path, F_OK) == 0)
        return true;
    return false;
#endif
}

#endif /* BIL_IMPLEMENTATION */

#ifndef BIL_STRIP_NAMESPACE
#define BIL_STRIP_NAMESPACE

typedef Bil_Cstr_Array      Cstr_Array;
typedef Bil_String_Builder  String_Builder;
typedef Bil_String_View     String_View;
typedef Bil_Cmd             Cmd;
typedef Bil_Dep             Dep;

#define EXIT_OK             BIL_EXIT_OK
#define EXIT_ERR            BIL_EXIT_ERR

#define DEP_UPDATE_TRUE     BIL_DEP_UPDATE_TRUE
#define DEP_UPDATE_FALSE    BIL_DEP_UPDATE_FALSE

#define WORKFLOW_NO_TIME    BIL_WORKFLOW_NO_TIME

#define Str_Args Bil_Str_Args
#define Str_Fmt  Bil_Str_Fmt

#define SCRIPT_REBUILD          BIL_SCRIPT_REBUILD
#define da_append               bil_da_append
#define da_append_many          bil_da_append_many
#define da_append_da            bil_da_append_da
#define da_clean                bil_da_clean
#define da_append_items         bil_da_append_items
#define report                  bil_report
#define sb_join_many            bil_sb_join_many
#define sb_join_cstr            bil_sb_join_cstr
#define sb_from_cstr            bil_sb_from_cstr
#define sb_join_nul             bil_sb_join_nul
#define sb_join                 bil_sb_join
#define sv_from_cstr            bil_sv_from_cstr
#define sv_cut_value            bil_sv_cut_value
#define sv_chop_by_space        bil_sv_chop_by_space
#define sv_cut_space_left       bil_sv_cut_space_left
#define char_in_sv              bil_char_in_sv
#define sv_cmp                  bil_sv_cmp
#define sv_to_u32               bil_sv_to_u32
#define sv_to_cstr              bil_sv_to_cstr
#define cmd_run_async           bil_cmd_run_async
#define cmd_run_sync            bil_cmd_run_sync
#define proc_await              bil_proc_await
#define dep_ischange            bil_dep_ischange
#define deps_ischange           bil_deps_ischange
#define shift_args              bil_shift_args
#define makedir                 bil_makedir
#define dir_exist               bil_dir_exist
#define mk_path                 bil_mk_path
#define file_exist              bil_file_exist
#define delete_file             bil_delete_file
#define read_file               bil_read_file
#define read_entire_file        bil_read_entire_file
#define rename_file             bil_rename_file
#define cut_file_extension      bil_cut_file_extension
#define replace_file_extension  bil_replace_file_extension
#define workflow_begin          bil_workflow_begin
#define workflow_end            bil_workflow_end
#define cmd_append              bil_cmd_append
#define cmd_clean               bil_cmd_clean
#define cmd_reset               bil_cmd_reset
#define cmd_append_cmd          bil_cmd_append_cmd
#define check_deps              bil_check_deps
#define dep_init                bil_dep_init
#define PATH                    BIL_PATH
#define defer_status            bil_defer_status

#endif // BIL_STRIP_NAMESPACE
