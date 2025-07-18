#include "lexer.h"

static const char *keywords[] = {
    "if",   "else", "elif", "while", "int",  "uint",
    "flt",  "str",  "char", "bool",  "void", "fn",
    "namespace", "module", "import", "return"
};

// TODO: delete for not debug version
void hmdump(Map *m) {
    for (size_t i = 0; i < m->capacity; ++i) {
        map_header itm = m->items[i];
        if (itm.hash != 0) {
            printf("Found keyword %u\n", *(Token_Type*)(itm.data));
        }
    }
}

#define lexer_current_next(l) \
    ({ (l)->current = *((l)->content.items + 1); })

Lexer lexer_init(String_View *src, const char *file_path) {
    Lexer l = {0};
    l.file = file_path;
    l.content = *src;
    sv_trim(&l.content);
    l.current = *l.content.items;
    l.line_start = src->items;
    l.line_number = l.status = 1;
    hminit(&l.keywords, TKC*3);
    size_t i = TK_IF;
    for (; i <= TK_RETURN; ++i)
        hmins(&l.keywords, keywords[i - TK_IF], &i);
    i = TK_NULL;
    hmins(&l.keywords, "null", &i);
    return l;
}

// TODO: Support 0d 0x formats
ALF_FUNC String_View tokenize_numlit(String_View *content) {
    size_t i = 0;
    while (i <= content->count && isdigit(content->items[i])) ++i;
    String_View result = sv_from_parts(content->items, i);
    sv_shift_left(content, i);
    return result;
}

ALF_FUNC String_View tokenize_text(String_View *content) {
    size_t i = 0;
    while (i <= content->count
        && (isalnum(content->items[i])
        ||  content->items[i] == '-'
        ||  content->items[i] == '_')
        && !isspace(content->items[i])) ++i;
    String_View result = sv_from_parts(content->items, i);
    sv_shift_left(content, i);
    return result;
}

#define typecase(type_, shift_) \
    ({ tk.type = (type_); tk.content.items = l->content.items; \
        sv_shift_left(&l->content, (shift_)); l->current = *l->content.items; return tk; })

ALF_FUNC Token tokenizer(Lexer *l) {
    Token tk;
    for (;;) {
        if (l->content.count == 0) {
            tk.type = TK_EOF;
            return tk;
        }
        switch (l->current) {
            case '#': {
                size_t i = 0;
                lexer_current_next(l);
                if (l->current == '>') {
                    while (i <= l->content.count) {
                        if (l->content.items[i] == '<'
                        &&  i + 1 <= l->content.count) {
                            if (l->content.items[i + 1] == '#') {
                                ++i; break;
                            } else ++i;
                        } else ++i;
                    }
                } else {
                    while (i <= l->content.count
                    &&  l->content.items[i] != '\n'
                    &&  l->content.items[i] != '\r') ++i;
                }
                sv_shift_left(&l->content, i);
                l->current = *l->content.items;
                break; 
            } case ' ': case '\f': case '\v': case '\t': { // Spaces
                sv_trim(&l->content) ;
                l->current = *l->content.items;
                break;
            } case '\n': case '\r': {
                lexer_current_next(l);
                sv_shift_left(&l->content, 1);
                l->line_start = l->content.items;
                l->line_number++;
                break;
            } case '+': {
                typecase(TK_PLUS, 1);
            } case '*': {
                typecase(TK_STAR, 1);
            } case '/': {
                typecase(TK_DIVIDE, 1);
            } case '%': {
                typecase(TK_PERCENT, 1);
            } case ':': {
                typecase(TK_COLON, 1);
            } case ',': {
                typecase(TK_COMMA, 1);
            } case '.': {
                typecase(TK_DOT, 1);
            } case '(': {
                typecase(TK_OPAREN, 1);
            } case ')': {
                typecase(TK_CPAREN, 1);
            } case '[': {
                typecase(TK_OBRACKET, 1);
            } case ']': {
                typecase(TK_CBRACKET, 1);
            } case '-': {
                lexer_current_next(l);
                if (l->current == '>')
                    typecase(TK_ARROW, 2);
                typecase(TK_MINUS, 1);
            } case '=': {
                lexer_current_next(l);
                if (l->current == '=')
                    typecase(TK_EQ, 2);
                typecase(TK_ASSIGN, 1);
            } case '<': {
                lexer_current_next(l);
                if (l->current == '=')
                    typecase(TK_LE, 2);
                typecase(TK_LT, 1);
            } case '>': {
                lexer_current_next(l);
                if (l->current == '=')
                    typecase(TK_GE, 2);
                typecase(TK_GT, 1);
            } case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9': {
                tk.content = tokenize_numlit(&l->content);
                tk.type = TK_NUMLIT;
                l->current = *l->content.items;
                return tk;
            } case '\"': case '\'':  {
                // TODO: String interpolation
                size_t i = 0;
                sv_shift_left(&l->content, 1); // Skip `"`
                while (i <= l->content.count
                &&     l->content.items[i] != '"') ++i;
                tk.content = sv_from_parts(l->content.items, i);
                tk.type = TK_STRLIT;
                sv_shift_left(&l->content, i + 1);
                l->current = *l->content.items;
                return tk;
            } default: {
                Token_Type *type;
                tk.content = tokenize_text(&l->content);
                l->current = *l->content.items;
                // TODO: Make `hmget` that except string length
                hmget_(&l->keywords, tk.content.items, tk.content.count, type);
                if (type == NULL) tk.type = TK_ID;
                else tk.type = *type;
                return tk;
            }
        }
    }
}

Token lexer_token_next(Lexer *l) {
    Token tk = tokenizer(l);
    tk.loc.offset = (size_t)(tk.content.items - l->line_start) + 1;
    tk.loc.line = l->line_number;
    tk.loc.file = l->file;
    return tk;
}

Token lexer_token_peek(Lexer *l) {
    (void)l;
    return (Token) { .type = TK_EOF };
}

Token lexer_token_expect(Lexer *l, Token_Type type) {
    (void)l; (void)type;
    return (Token) { .type = TK_EOF };
}

ALF_FUNC const char *token_type_as_cstr(Token_Type t) {
    switch (t) {
        case TK_NULL:      return "null";
        case TK_NUMLIT:    return "numlit"; 
        case TK_STRLIT:    return "strlit"; 
        case TK_ID:        return "id"; 
        case TK_ARROW:     return "arrow"; 
        case TK_OPAREN:    return "oparen"; 
        case TK_CPAREN:    return "cparen"; 
        case TK_OBRACKET:  return "obracket"; 
        case TK_CBRACKET:  return "cbracket"; 
        case TK_PLUS:      return "plus"; 
        case TK_MINUS:     return "minus"; 
        case TK_STAR:      return "star"; 
        case TK_PERCENT:   return "percent"; 
        case TK_DIVIDE:    return "divide"; 
        case TK_COMMA:     return "comma"; 
        case TK_COLON:     return "colon"; 
        case TK_DOT:       return "dot"; 
        case TK_ASSIGN:    return "assign"; 
        case TK_EQ:        return "eq"; 
        case TK_LE:        return "le"; 
        case TK_LT:        return "lt"; 
        case TK_GE:        return "ge"; 
        case TK_GT:        return "gt"; 
        case TK_IF:        return "if"; 
        case TK_ELSE:      return "else"; 
        case TK_ELIF:      return "elif"; 
        case TK_WHILE:      return "while"; 
        case TK_INT:      return "int"; 
        case TK_UINT:      return "uint"; 
        case TK_FLT:      return "flt"; 
        case TK_STR:      return "str"; 
        case TK_CHAR:      return "char"; 
        case TK_BOOL:      return "bool"; 
        case TK_VOID:      return "void"; 
        case TK_FN:      return "fn"; 
        case TK_NMSPC:      return "nmspc"; 
        case TK_MODULE:      return "module"; 
        case TK_IMPORT:      return "import"; 
        case TK_RETURN:      return "return"; 
        case TK_EOF:       return "eof";
        case TK_NONE:   return "none";
        default: {
            return "unknown token type";
        }
    }
}

void lexer_token_print(Token *tk) {
    printf("Token[%s]:%zu:%zu ", token_type_as_cstr(tk->type), tk->loc.line, tk->loc.offset);
    switch (tk->type) {
        case TK_NULL:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_NUMLIT:     printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_STRLIT:     printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_ID:         printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_ARROW:      printf("'->'"); break;
        case TK_OPAREN:     printf("'('"); break;
        case TK_CPAREN:     printf("')'"); break;
        case TK_OBRACKET:   printf("'['"); break;
        case TK_CBRACKET:   printf("']'"); break;
        case TK_PLUS:       printf("'+'"); break;
        case TK_MINUS:      printf("'-'"); break;
        case TK_STAR:       printf("'*'"); break;
        case TK_PERCENT:    printf("'percent'"); break;
        case TK_DIVIDE:     printf("'/'"); break;
        case TK_COMMA:      printf("','"); break;
        case TK_COLON:      printf("':'"); break;
        case TK_DOT:        printf("'.'"); break;
        case TK_ASSIGN:     printf("'='"); break;
        case TK_EQ:         printf("'=='"); break;
        case TK_LE:         printf("'<='"); break;
        case TK_LT:         printf("'>'"); break;
        case TK_GE:         printf("'>='"); break;
        case TK_GT:         printf("'>'"); break;
        case TK_IF:         printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_ELSE:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_ELIF:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_WHILE:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_INT:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_UINT:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_FLT:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_STR:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_CHAR:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_BOOL:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_VOID:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_FN:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_NMSPC:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_MODULE:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_IMPORT:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_RETURN:       printf("'"Str_Fmt"'", Str_Args(tk->content)); break;
        case TK_EOF:        printf("'eof'"); break;
        default: {
            printf("Unknown token type `%u`", tk->type);
            break;
        }
    }
    printf("\n");
}
