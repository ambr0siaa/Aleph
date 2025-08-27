#include <assert.h>
#include "lexer.h"
#include "core.h"

/*
 *      Utils
 */

const char *builtin_types[BUILTIN_TYPES_COUNT] = {
    "void", "int", "uint", "flt", "bool", "char"
};

#define ENTRY_FUNCTION (CString) { .items = "main", .count = 4 }


#define context_push_inst(a, c, o) \
    arena_da_append_parts(a, c->code, c->codeptr, c->codesize, (Instruction)(o))

#define context_push_data(a, c, v) \
    arena_da_append_parts(a, c->data, c->dataptr, c->datasize, v)

#define funcall_arg(a, f, arg) arena_da_append(a, f, arg)
#define funcall_init(a) arena_alloc(a, sizeof(Funcall))

#define alf_inst_init(a, alf) do {\
    alf->code = arena_alloc(a, sizeof(void*)); \
    alf->data = arena_alloc(a, sizeof(void*)); \
} while(0)

static inline Module *globmod_init(void) {
    Arena a = {0};
    Module *m = arena_alloc(&a, sizeof(Module));
    map_init(&m->typetable, BUILTIN_TYPES_COUNT*3);
    Fundef *putstr = arena_alloc(&a, sizeof(Fundef));
    putstr->bltin = 1;
    map_init(&m->callstack, 8);
    mapins(&m->callstack, "putstr", 6, putstr);
    for (size_t i = 0; i < BUILTIN_TYPES_COUNT; ++i)
        map_ins(&m->typetable, builtin_types[i], &i);
    m->entry = 1;
    m->a = a;
    return m;
}

static Context *context_init(Module *m) {
    Context *c = arena_alloc(&m->a, sizeof(Context));
    c->codesize = c->datasize = 0;
    c->codeptr = c->dataptr = 0;
    c->code = NULL;
    c->data = NULL;
    c->module = m;
    return c;
}

static inline Fundef *fundef_search(Module *m, CString *name) {
    Fundef *f = NULL;
    map_strget(&m->callstack, name->items, name->count, f);
    return f;
}

static inline alf_uint *typeget(Map *t, CString *key) {
    alf_uint *id;
    map_strget(t, key->items, key->count, id);
    return id;
}

static inline int checktype(Map *m, CString *name) {
    alf_uint *id;
    map_strget(m, name->items, name->count, id);
    if (!id) return 0;
    else return 1;
}

static void alf_load_func(Arena *a, Alf_State *alf, Fundef *f) {
    if (!f) return;
    Context *c = f->c;
    f->addr = alf->codeptr;
    if (!alf->code) {
        assert(!alf->data);
        alf_inst_init(a, alf);
    }
    arena_da_append_parts(a, alf->code, alf->codeptr, alf->codesize, c->code);
    arena_da_append_parts(a, alf->data, alf->dataptr, alf->datasize, c->data);
    assert(alf->codeptr == alf->dataptr);
}

/*
 *      Expression
 */

static void vardeclare(Lexer *l, Token *type, Token *name) {
    (void)name;
    l->alf->status = ALF_STATUS_FCK;
    panic(&type->loc, "TODO: variable declararion not implemented");
    return;
}

static void variable(Lexer *l, Token *type, Token *name) {
    vardeclare(l, type, name);
    return;
}

static Funcall *funcall(Arena *a, Lexer *l, CString *name) {
    Funcall *f = funcall_init(a);
    f->name = *name;
    lexer_expect(l, TK_OPAREN);
    if (lexer_failer(l)) return NULL;
    Token tk;
    for (;;) {
        tk = lexer_peek(l);
        if (tk.type == TK_CPAREN) {
            lexer_next(l);
            break;
        } else if (tk.type == TK_COMMA) {
            lexer_next(l);
        }
        Expr arg = expr(a, l);
        funcall_arg(a, f, arg);
    }
    lexer_expect(l, TK_SEMICOLON);
    return f;
}

Expr expr(Arena *a, Lexer *l) {
    Expr e = {0};
    Token tk = lexer_next(l);
    CString text = tk.text;
    switch (tk.type) {
        case TK_STRLIT: {
            e.t = EXPR_STR;
            e.s = string(a, (char*)text.items, text.count);
            break;
        }
        case TK_ID: {
            e.t = EXPR_FUNCALL;
            e.fc = funcall(a, l, &text);
            if (!e.fc) e = ExprNone; 
            break;
        }
        default: {
            panic(&tk.loc, "Unknown expression started from token `%s`", 
                  token_type_as_cstr(tk.type));
            l->alf->status = ALF_STATUS_FCK;
            e = ExprNone;
        }
    }
    return e;
}

/*
 *      Parser
 */

static void fundef(Module *m, Lexer *l, Token *t, CString *name) {
    Fundef f = {0};
    Alf_State *alf = l->alf;
    Map *typetable = &m->typetable;
    alf_uint *type = typeget(typetable, &t->text);
    if (type == NULL) {
        alf->status = ALF_STATUS_FCK;
        panic(&t->loc, "Unknown type `"Str_Fmt"`", 
              Str_Args(t->text));
        return;
    }
    f.type = *type;
    lexer_expect(l, TK_OPAREN);
    lexer_defer;
    // TODO: Parse arguments
    lexer_expect(l, TK_CPAREN);
    lexer_defer;
    if (lexer_check(l, TK_SEMICOLON)) {
        lexer_next(l); // Skip semicolon
        panic(&t->loc, "TODO: not implemented function declaration");
        alf->status = ALF_STATUS_FCK;
        return;
    }
    Context *body = context_init(m);
    f.c = body;
    lexer_expect(l, TK_OCURLY);
    lexer_defer;
    parse_context(body, l);
    if (str_cmp(name, &ENTRY_FUNCTION)) {
        context_push_inst(&m->a, body, OP_HLT);
    } else {
        context_push_inst(&m->a, body, OP_RET);
    }
    mapins(&m->callstack,
           name->items,
           name->count, &f);
    lexer_expect(l, TK_CCURLY);
    defer: {
        return;
    }
}

static void parse_expr(Context *c, Lexer *l) {
    Module *m = c->module;
    Alf_State *alf = l->alf;
    Token tk = lexer_peek(l);
    Expr e = expr(&m->a, l);
    switch (e.t) {
        case EXPR_FUNCALL: {
            Funcall *fc = e.fc;
            Fundef *f = fundef_search(m, &fc->name);
            if (f == NULL) {
                alf->status = ALF_STATUS_FCK;
                alf_panic(Err_Fmt,
                          "Call to not define function `"Str_Fmt"`",
                          Str_Args(fc->name));
                return;
            }
            if (!f->bltin) {
                context_push_inst(&m->a, c, OP_CALL);
                if (!f->used) {
                    f->used = 1;
                    alf_load_func(&m->a, alf, f);
                }
            } else {
                context_push_inst(&m->a, c, OP_BLTIN);
            }
            context_push_data(&m->a, c, StkUint(f->addr));
            for (size_t i = 0; i < fc->count; ++i) {
                Expr arg = fc->items[i];
                switch (arg.t) {
                    case EXPR_STR: {
                        context_push_data(&m->a, c, StkStr(arg.s));
                        break;
                    } default: {
                        alf->status = ALF_STATUS_FCK;
                        panic(&tk.loc, "TODO: Not supported another availible"
                                       "types for functions arguments");
                        return;
                    }
                }
            }
            break;
        }
        default: {
            alf->status = ALF_STATUS_FCK;
            panic(&tk.loc, "Supported only funcalls as"
                           "single parsing expression");
            return;
        }
    }
}

void parse_statement(Context *c, Lexer *l) {
    Alf_State *alf = l->alf;
    Token fst = lexer_peek(l);
    switch (fst.type) {
        case TK_ID: {
            parse_expr(c, l);
            break;
        }
        case TK_IF: {
            panic(&fst.loc, "TODO: `if` stmt not implemented");
            break; 
        }
        case TK_WHILE: {
            panic(&fst.loc, "TODO: `while` stmt not implemented");
            break; 
        }
        case TK_RETURN: {
            panic(&fst.loc, "TODO: `return` stmt not implemented");
            break; 
        }
        default: {
            alf->status = ALF_STATUS_FCK;
            panic(&fst.loc, "Unknown statement started"
                  "from token `"Str_Fmt"`", fst.text);
            return;
        }
    }
}

static void parse_primary_stmt(Module *m, Lexer *l) {
    Alf_State *alf = l->alf;
    Token fst = lexer_peek(l);
    switch (fst.type) {
        case TK_ID: {
            if (checktype(&m->typetable, &fst.text)) {
                lexer_next(l); // Skip type
                Token name = lexer_expect(l, TK_ID);
                if (lexer_failer(l)) {
                    panic(&name.loc,
                          "Expected name of function definition,"
                          "but provided token:%s:\""Str_Fmt"\"",
                          token_type_as_cstr(name.type),
                          Str_Args(name.text));
                    goto defer;
                }
                fundef(m, l, &fst, &name.text);
            } else {
                panic(&fst.loc, "Expected type for function definition");
                goto defer;
            }
            break;
        }
        case TK_MODULE: {
            panic(&fst.loc, "TODO: `module` stmt not implemented");
            break; 
        }
        case TK_IMPORT: {
            panic(&fst.loc, "TODO: `import` stmt not implemented");
            break; 
        }
        default: {
            panic(&fst.loc,
                  "Expected module | import | function definition."
                  "Provided token:%s:\""Str_Fmt"\" for unknown"
                  "primary statement", token_type_as_cstr(fst.type),
                  Str_Args(fst.text));
            defer: {
                alf->status = ALF_STATUS_FCK;
                return;
            }
        }
    }

}

void parse_context(Context *c, Lexer *l) {
    for (;;) {
        parse_statement(c, l);
        if (lexer_failer(l)) return;
        Token tk = lexer_peek(l);
        if (tk.type == TK_EOF
        ||  tk.type == TK_CCURLY) break;
    }
}

static void parse_module(Module *m, Lexer *l) {
    for (;;) {
        parse_primary_stmt(m, l);
        if (lexer_failer(l)) return;
        Token tk = lexer_peek(l);
        if (tk.type == TK_EOF) break;
    }
}

/*
 *      Start work
 */

static inline void entry_point(Alf_State *a, Module *m) {
    Fundef *entry = fundef_search(m, &ENTRY_FUNCTION);
    if (!entry) {
        a->status = ALF_STATUS_FCK;
        alf_panic(Err_Fmt, "Could not find entry point."
                           "Expected declaration of `main` function");
    } else {
        if (!a->code) {
            alf_assert(!a->data);
            alf_inst_init(&m->a, a);
        }
        a->code[0] = entry->c->code;
        a->data[0] = entry->c->data;
    }
}

void mainfunc(Alf_State *a, Reader *r) {
    Lexer l = lexer_init(a, r);
    Module *m = globmod_init();
    parse_module(m, &l);
    alf_badcase(a); // Bad parsing?
    entry_point(a, m);
    alf_badcase(a); // No entry?
    vm_run_program(a);
    defer: {
        map_free(&m->callstack);
        map_free(&m->typetable);
        map_free(&l.keywords);
        arena_free(&m->a);
    }
}
