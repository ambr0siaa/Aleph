#ifndef LEXER_H_
#define LEXER_H_

#include "common.h"
#include "hashmap.h"

// TODO: Support bitwise operation, logic operation, operation when assigning
typedef enum {
    TK_NONE = 0, TK_NULL, TK_NUMLIT, TK_STRLIT, TK_ID,
    
    // Special tokens
    TK_ARROW, TK_OPAREN, TK_CPAREN, TK_OBRACKET, TK_CBRACKET,
    TK_PLUS, TK_MINUS, TK_STAR, TK_PERCENT, TK_DIVIDE, TK_COMMA,
    TK_COLON, TK_DOT, TK_ASSIGN, TK_EQ, TK_LE, TK_LT, TK_GE, TK_GT,
    
    // Keywords: only in this order
    TK_IF, TK_ELSE, TK_ELIF, TK_WHILE, TK_INT, TK_UINT,
    TK_FLT, TK_STR, TK_CHAR, TK_BOOL, TK_VOID, TK_FN,
    TK_NMSPC, TK_MODULE, TK_IMPORT, TK_RETURN,

    TK_EOF,
    TKC     // Count of Tokens
} Token_Type;

#define lexer_failer(l) ((l)->status == 0)

typedef struct {
    Token_Type type;
    String_View content;
    Location loc;
} Token;

#define TOK_NONE (Token) { .type = TK_NONE }

typedef struct {
    Map keywords;           // Table of keywords
    size_t status;          // State of the lexer 
    char current;           // First character
    String_View content;    // Source code
    size_t line_number;     // Number of line
    const char *line_start; // For offset
    const char *file;
} Lexer;

ALF_API Lexer lexer_init(String_View *src, const char *file_path);
ALF_API Token lexer_token_next(Lexer *l);
ALF_API Token lexer_token_peek(Lexer *l);
ALF_API Token lexer_token_expect(Lexer *l, Token_Type type);
ALF_API void  lexer_token_print(Token *tk);

#endif // LEXER_H_
