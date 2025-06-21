#ifndef LEXER_H_
#define LEXER_H_

#include "common.h"

typedef enum {
    TK_NONE = 0,
    TK_KEYWORD,
    TK_MARKS,
    TK_NIL,
    TK_NUM,
    TK_LIT,
    TK_OP,
    TK_ID,
    TK_EOF,
    TKC     // Count of Tokens
} Token_Type;

typedef struct {
    Location loc;
    Token_Type type;
    String_View content;
} Token;

#endif // LEXER_H_
