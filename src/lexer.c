#include "lexer.h"
#include "panic.h"

static const char *keywords[] = {
    "if", "else", "elif", "while", "true",
    "false", "module", "import", "return"
};

#define lexer_current_next(l) \
    do { (l)->current = *((l)->content.items + 1); } while(0)

inline Lexer lexer_init(Alf_State *a, Reader *r) {
    Lexer l = {0};
    l.alf = a;
    l.file = r->file;
    l.content = reader_slice(r);
    slice_trim(&l.content);
    l.line_start = l.content.items;
    l.current = *l.line_start;
    l.line_number = 1;
    map_init(&l.keywords, TKC*3);
    for (size_t i = TK_IF; i <= TK_RETURN; ++i)
    map_ins(&l.keywords, keywords[i - TK_IF], &i);
    return l;
}

static Slice tokenize_numlit(Slice *content) {
    size_t i = 0;
    const char *items = content->items;
    while (i <= content->count && isdigit(items[i])) ++i;
    Slice result = slice_parts(items, i);
    slice_shift(content, i);
    return result;
}

static Slice tokenize_text(Slice *content) {
    size_t i = 0;
    const char *items = content->items;
    while (i <= content->count
    &&    (isalnum(items[i])
    ||    items[i] == '-'
    ||    items[i] == '_')
    &&    !isspace(items[i])) ++i;
    Slice result = slice_parts(items, i);
    slice_shift(content, i);
    return result;
}

#define typecase(type_, shift_) do { \
    tk.type = (type_); \
    tk.text.count = (shift_); \
    tk.text.items = l->content.items; \
    slice_shift(&l->content, (shift_)); \
    l->current = *l->content.items; \
    return tk; \
} while (0)

#define charisret(c) ((c) == '\n' || (c) == '\r')

static Token tokenizer(Lexer *l) {
    Token tk;
    for (;;) {
        if (l->content.count == 0) {
            tk.type = TK_EOF;
            return tk;
        }
        switch (l->current) {
            case '+': typecase(TK_PLUS, 1);
            case '*': typecase(TK_STAR, 1);
            case ',': typecase(TK_COMMA, 1);
            case ';': typecase(TK_SEMICOLON, 1);
            case '.': typecase(TK_DOT, 1);
            case '(': typecase(TK_OPAREN, 1);
            case ')': typecase(TK_CPAREN, 1);
            case '[': typecase(TK_OBRACKET, 1);
            case ']': typecase(TK_CBRACKET, 1);
            case '{': typecase(TK_OCURLY, 1);
            case '}': typecase(TK_CCURLY, 1);
            case '-': typecase(TK_MINUS, 1);
            case ':': {
                lexer_current_next(l);
                if (l->current == ':')
                    typecase(TK_DBLCOLON, 2);
                typecase(TK_COLON, 1);
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
            } case '/': {
                size_t i = 0;
                size_t count = l->content.count;
                const char *items = l->content.items;
                lexer_current_next(l);
                if (l->current == '*') {
                    // Multi-comments
                    while (i < count) {
                        if (items[i] == '*'
                        &&  i + 1 < count) {
                            if (items[i + 1] == '/') {
                                i += 2; break;
                            } else ++i;
                        } else if (charisret(items[i])) {
                            l->line_number++; ++i; 
                        } else ++i;
                    }
                } else if (l->current == '/') {
                    // Single line comment
                    while (i <= count
                    &&    !charisret(items[i])) ++i;
                } else {
                    typecase(TK_DIVIDE, 1);
                }
                slice_shift(&l->content, i);
                l->current = *l->content.items;
                break; 
            } case ' ': case '\f': case '\v': case '\t': { // Spaces
                slice_trim(&l->content);
                l->current = *l->content.items;
                break;
            } case '\n': case '\r': {
                Slice *content = &l->content;
                lexer_current_next(l);
                slice_shift(content, 1);
                l->line_start = content->items;
                l->line_number++;
                break;
            } case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9': {
                tk.text = tokenize_numlit(&l->content);
                tk.type = TK_NUMLIT;
                l->current = *l->content.items;
                return tk;
            } case '\"': case '\'':  {
                size_t i = 0;
                Slice *content = &l->content;
                slice_shift(content, 1); // Skip `"`
                while (i <= content->count) {
                    if (content->items[i] == '"') break;
                    else if (content->items[i] == '\\') i += 2;
                    else ++i;
                }
                tk.type = TK_STRLIT;
                tk.text = slice_parts(content->items, i);
                slice_shift(content, i + 1);
                l->current = *content->items;
                return tk;
            } default: {
                Token_Type *type;
                CString *text = &tk.text;
                *text = tokenize_text(&l->content);
                if (text->count == 0) {
                    l->alf->status = ALF_STATUS_FCK;
                    return tk;
                }
                l->current = *l->content.items;
                map_strget(&l->keywords, text->items, text->count, type);
                if (type == NULL) tk.type = TK_ID;
                else tk.type = *type;
                return tk;
            }
        }
    }
}

Token lexer_next(Lexer *l) {
    Token tk = tokenizer(l);
    tk.loc.line = l->line_number;
    tk.loc.offset = (size_t)(tk.text.items - l->line_start) + 1;
    tk.loc.file = l->file;
    return tk;
}

Token lexer_peek(Lexer *l) {
    Lexer copy = *l;
    return lexer_next(&copy);
}

Token lexer_expect(Lexer *l, Token_Type type) {
    Token tk = lexer_next(l);
    if (tk.type != type) {
        panic(&tk.loc, "Exptected token `%s` but provided `%s`", 
              token_type_as_cstr(type), token_type_as_cstr(tk.type));
        l->alf->status = ALF_STATUS_FCK;
        return TOK_NONE;
    } else return tk;
}

int lexer_check(Lexer *l, Token_Type type) {
    Token tk = lexer_peek(l);
    if (tk.type == type) return 1;
    else return 0;
}

const char *token_type_as_cstr(Token_Type t) {
    switch (t) {
        case TK_SEMICOLON: return "semicolon";
        case TK_NUMLIT:    return "numlit"; 
        case TK_STRLIT:    return "strlit"; 
        case TK_ID:        return "id"; 
        case TK_OPAREN:    return "open paren"; 
        case TK_CPAREN:    return "close paren"; 
        case TK_OBRACKET:  return "open bracket"; 
        case TK_CBRACKET:  return "close bracket"; 
        case TK_OCURLY:    return "open curly bracket"; 
        case TK_CCURLY:    return "close curly brecket"; 
        case TK_PLUS:      return "plus"; 
        case TK_MINUS:     return "minus"; 
        case TK_STAR:      return "star/multiply"; 
        case TK_DIVIDE:    return "slash/divide"; 
        case TK_COMMA:     return "comma"; 
        case TK_COLON:     return "colon"; 
        case TK_DBLCOLON:  return "double colon"; 
        case TK_DOT:       return "dot"; 
        case TK_ASSIGN:    return "assign"; 
        case TK_EQ:        return "equal"; 
        case TK_LE:        return "less or equal"; 
        case TK_LT:        return "less then"; 
        case TK_GE:        return "greater or equal"; 
        case TK_GT:        return "greater than"; 
        case TK_IF:        return "if"; 
        case TK_ELSE:      return "else"; 
        case TK_ELIF:      return "elif"; 
        case TK_WHILE:     return "while"; 
        case TK_MODULE:    return "module"; 
        case TK_IMPORT:    return "import"; 
        case TK_RETURN:    return "return"; 
        case TK_EOF:       return "eof";
        case TK_NONE:      return "none";
        case TK_FALSE:
        case TK_TRUE:      return "bool";
        default: {
            return "unknown token type";
        }
    }
}

void lexer_token_print(Token *tk) {
    printf("Token[%s]:%zu:%zu ",
           token_type_as_cstr(tk->type),
           tk->loc.line, tk->loc.offset);
    switch (tk->type) {
        case TK_OCURLY:     printf("'{'");   break;
        case TK_CCURLY:     printf("'}'");   break;
        case TK_OPAREN:     printf("'('");   break;
        case TK_CPAREN:     printf("')'");   break;
        case TK_OBRACKET:   printf("'['");   break;
        case TK_CBRACKET:   printf("']'");   break;
        case TK_PLUS:       printf("'+'");   break;
        case TK_MINUS:      printf("'-'");   break;
        case TK_STAR:       printf("'*'");   break;
        case TK_DIVIDE:     printf("'/'");   break;
        case TK_COMMA:      printf("','");   break;
        case TK_SEMICOLON:  printf("';'");   break;
        case TK_COLON:      printf("':'");   break;
        case TK_DBLCOLON:   printf("'::'");   break;
        case TK_DOT:        printf("'.'");   break;
        case TK_ASSIGN:     printf("'='");   break;
        case TK_EQ:         printf("'=='");  break;
        case TK_LE:         printf("'<='");  break;
        case TK_LT:         printf("'>'");   break;
        case TK_GE:         printf("'>='");  break;
        case TK_GT:         printf("'>'");   break;
        case TK_EOF:        printf("'eof'"); break;
        case TK_NUMLIT: case TK_STRLIT: case TK_TRUE:
        case TK_FALSE:  case TK_ID:     case TK_IF:
        case TK_ELSE:   case TK_ELIF:   case TK_WHILE:
        case TK_MODULE: case TK_IMPORT: case TK_RETURN:
            printf("'"StrFmt"'", StrArgs(tk->text)); break;
        default: {
            printf("Unknown token type `%u`", tk->type);
            break;
        }
    }
    printf("\n");
}

void lexer_view(Lexer *l) {
    for (;;) {
        Token tk = lexer_next(l);
        if (lexer_failer(l)) {
            fprintf(stderr, "%zu:%zu:Unknown character `%c`",
                    tk.loc.line, tk.loc.offset, l->current);
            return;
        }
        if (tk.type == TK_EOF) break;
        lexer_token_print(&tk);
    }
}

