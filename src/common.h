#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <assert.h>

#include "str.h"

#define ALF_API  extern
#define ALF_FUNC static inline

#define ALF_STATUS_OK  0
#define ALF_STATUS_FCK 1

typedef struct {
    size_t line, offset;
    const char *file;
} Location;

typedef struct {
    int status;
    String_View source;
    const char *src_path;
    const char *program;
} Alf_State;

#endif // COMMON_H_
