#ifndef LEXER_H_
#define LEXER_H_

#include "common.h"
#include "reader.h"
#include "hashmap.h"

// TODO: Support bitwise operation, logic operation, operation when assigning
typedef enum {
    TK_NONE = 0, TK_NUMLIT, TK_STRLIT, TK_ID,
    
    // Special tokens
    TK_OPAREN, TK_CPAREN, TK_OBRACKET, TK_CBRACKET, TK_PLUS,
    TK_MINUS, TK_STAR, TK_DIVIDE, TK_COMMA, TK_OCURLY, TK_CCURLY,
    TK_SEMICOLON, TK_COLON, TK_DOT, TK_ASSIGN, TK_EQ, TK_LE, TK_LT, TK_GE, TK_GT,
    
    // Keywords: only in this order
    TK_IF, TK_ELSE, TK_ELIF, TK_WHILE, TK_INT, TK_UINT,
    TK_FLT, TK_CHAR, TK_BOOL, TK_VOID, TK_TRUE,
    TK_FALSE, TK_MODULE, TK_IMPORT, TK_RETURN,

    TK_EOF,
    TKC     // Count of Tokens
} Token_Type;

#define lexer_failer(l) ((l)->alf->status == ALF_STATUS_FCK)

typedef struct {
    Token_Type type;
    Location   loc;
    Slice      text;
} Token;

#define TOK_NONE (Token) { .type = TK_NONE }

typedef struct {
    Map        keywords;           // Table of keywords
    char       current;           // First character
    Slice      content;          // Source code
    alf_uint   line_number;   // Number of line
    const char *line_start; // For offset
    const char *file;       // Parsing file
    Alf_State  *alf;        // Save state
} Lexer;

ALF_API Lexer lexer_init(Alf_State *a, Reader *r);
ALF_API Token lexer_next(Lexer *l);
ALF_API Token lexer_peek(Lexer *l);
ALF_API Token lexer_expect(Lexer *l, Token_Type type);
ALF_API void  lexer_token_print(Token *tk);
ALF_API void  lexer_view(Lexer *l);
ALF_API const char *token_type_as_cstr(Token_Type t);
ALF_API int lexer_check(Lexer *l, Token_Type type);

#endif // LEXER_H_
