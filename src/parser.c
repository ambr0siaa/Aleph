#include "lexer.h"
#include "core.h"

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif // _WIN32

/*
 *      Utils
 */

#define BUILTIN_PATH        "src/builtin.so"
#define ENTRY_FUNCTION      (CString) { .items = "main", .count = 4 }
#define BUILTIN_TYPES_COUNT 6

#define funcall_arg(a, f, arg) arena_da_append(a, f, arg)

#define context_push_inst(a, c, o) \
    arena_da_append_parts(a, c->code, c->codeptr, c->codesize, (Instruction)(o))

#define context_push_data(a, c, v) \
    arena_da_append_parts(a, c->data, c->dataptr, c->datasize, v)

#define funcall_init(a) arena_alloc(a, sizeof(Funcall))

#define alf_inst_init(a, alf) do { \
    alf->code = arena_alloc(a, sizeof(void*)); \
    alf->data = arena_alloc(a, sizeof(void*)); \
} while(0)

static const char *builtin_types[BUILTIN_TYPES_COUNT] = {
    "void", "int", "uint", "flt", "bool", "char"
};

static inline Module *globmod_init(void) {
    Arena a = {0};
    Module *m = arena_alloc(&a, sizeof(Module));
    map_init(&m->callstack, 8);
    map_init(&m->typetable, BUILTIN_TYPES_COUNT*3);
    for (size_t i = 0; i < BUILTIN_TYPES_COUNT; ++i)
        map_ins(&m->typetable, builtin_types[i], &i);
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

static inline void unload_builtins(Alf_State *alf) {
    #ifndef _WIN32
    if (dlclose(alf->builtins) != 0) {
        panicf(ErrFmt, "Could not unload file %s : %s", BUILTIN_PATH, dlerror());
        alf->status = ALF_STATUS_FCK;
    }
    #else
    if (FreeLibrary(alf->builtins) == 0) {
        panicf(ErrFmt, "Could not unload file %s : %u", BUILTIN_PATH, GetLastError());
        alf->status = ALF_STATUS_FCK;
    }
    #endif  
}

static inline void load_builtins(Arena *a, Alf_State *alf) {
    #ifndef _WIN32
    void *handle = dlopen(BUILTIN_PATH, RTLD_LAZY);
    if (handle == NULL) {
        panicf(ErrFmt, "Could not could not load builtins by path `%s`: %s\n",
               BUILTIN_PATH, dlerror());
        alf->status = ALF_STATUS_FCK;
        return;
    }
    #else
    HMODULE handle = LoadLibraryA(path);
    if (handle == NULL) {
        panicf(ErrFmt, "Could not could not load builtins by path `%s`: %u",
               BUILTIN_PATH, GetLastError());
        alf->status = ALF_STATUS_FCK;
        return;
    }
    #endif
    alf->builtins = arena_alloc(a, sizeof(void*));
    alf->builtins = handle;
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

// Coming soon...
static void vardeclare(Lexer *l, Token *type, Token *name) {
    (void)name;
    l->alf->status = ALF_STATUS_FCK;
    panic(&type->loc, "TODO: variable declararion not implemented");
    return;
}

// Coming soon...
static void variable(Lexer *l, Token *type, Token *name) {
    vardeclare(l, type, name);
    return;
}

static Funcall *function_call(Arena *a, Lexer *l, CString *name) {
    Funcall *f = funcall_init(a);
    if (lexer_check(l, TK_DBLCOLON)) {
        Slice bltin = slice_parts("builtin", 7);
        if (!str_cmp(name, &bltin)) {
            panicf(TodoFmt, "For now only expected `builtin` module");
            l->alf->status = ALF_STATUS_FCK;
            return NULL; 
        }
        lexer_next(l); // Skip double colon
        Token tk = lexer_expect(l, TK_ID);
        if (lexer_failer(l)) return NULL;
        f->name = tk.text;
        f->bltin = 1;
    } else {
        f->name = *name;
    }
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
        } case TK_ID: {
            e.t = EXPR_FUNCALL;
            e.fc = function_call(a, l, &text);
            if (!e.fc) e = ExprNone; 
            break;
        } default: {
            panic(&tk.loc, "Unknown expression started from token `%s`", 
                  token_type_as_cstr(tk.type));
            e = ExprNone;
        }
    }
    return e;
}

/*
 *      Parser
 */

static void parse_expression(Context *c, Lexer *l) {
    Module *m = c->module;
    Alf_State *alf = l->alf;
    Token location = lexer_peek(l);
    Expr e = expr(&m->a, l);
    if (e.t == EXPR_NONE) {
        panic(&location.loc, "Undefine kind of expression have returned");
        alf_defer(alf, ALF_STATUS_FCK);
    }
    switch (e.t) {
        case EXPR_FUNCALL: {
            Funcall *fc = e.fc;
            if (fc->bltin) {
                // Note: Pushing funcall name for searching in loaded library
                context_push_inst(&m->a, c, OP_BLTIN);
                String *fname = string(&m->a,
                                       (char*)fc->name.items,
                                       fc->name.count);
                context_push_data(&m->a, c, StkStr(fname));
            } else {
                Fundef *f = fundef_search(m, &fc->name);
                if (f == NULL) {
                    panicf(ErrFmt, "Call to not define function `"StrFmt"`",
                           StrArgs(fc->name));
                    alf_defer(alf, ALF_STATUS_FCK);
                }
                if (!f->used) {
                    alf_load_func(&m->a, alf, f);
                    f->used = 1;
                }
                context_push_inst(&m->a, c, OP_CALL);
                context_push_data(&m->a, c, StkUint(f->addr));
            }
            for (size_t i = 0; i < fc->count; ++i) {
                Expr arg = fc->items[i];
                switch (arg.t) {
                    case EXPR_STR: {
                        context_push_data(&m->a, c, StkStr(arg.s));
                        break;
                    } default: {
                        panic(&location.loc, "TODO: Not supported another availible"
                                             "types for functions arguments");
                        alf_defer(alf, ALF_STATUS_FCK);
                    }
                }
            }
            break;
        } default: {
            panic(&location.loc, "Supported only funcalls as"
                                 "single parsing expression");
            alf_defer(alf, ALF_STATUS_FCK);
        }
    }
    defer: {
        return;
    }
}

static void parse_statement(Context *c, Lexer *l) {
    Alf_State *alf = l->alf;
    Token fst = lexer_peek(l);
    switch (fst.type) {
        case TK_ID: {
            parse_expression(c, l);
            break;
        } case TK_IF: {
            panic(&fst.loc, "TODO: `if` stmt not implemented");
            break; 
        } case TK_WHILE: {
            panic(&fst.loc, "TODO: `while` stmt not implemented");
            break; 
        } case TK_RETURN: {
            panic(&fst.loc, "TODO: `return` stmt not implemented");
            break; 
        } default: {
            alf->status = ALF_STATUS_FCK;
            panic(&fst.loc, "Unknown statement started"
                            " from token `"StrFmt"`", fst.text);
            return;
        }
    }
}

static void parse_context(Context *c, Lexer *l) {
    for (;;) {
        parse_statement(c, l);
        if (lexer_failer(l)) return;
        Token tk = lexer_peek(l);
        if (tk.type == TK_EOF
        ||  tk.type == TK_CCURLY) break;
    }
}

static void function_definition(Module *m, Lexer *l, Token *t, CString *name) {
    Fundef f = {0};
    Alf_State *alf = l->alf;
    Map *typetable = &m->typetable;

    { // declaration
        alf_uint *type = typeget(typetable, &t->text);
        if (type == NULL) {
            alf->status = ALF_STATUS_FCK;
            panic(&t->loc, "Unknown type `"StrFmt"`", 
                  StrArgs(t->text));
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
    }

    { // body
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
    }

    defer: {
        return;
    }
}

static void parse_prime_statement(Module *m, Lexer *l) {
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
                          "but provided token:%s:\""StrFmt"\"",
                          token_type_as_cstr(name.type),
                          StrArgs(name.text));
                    goto defer;
                }
                function_definition(m, l, &fst, &name.text);
            } else {
                panic(&fst.loc, "Expected type for function definition");
                goto defer;
            }
            break;
        } case TK_MODULE: {
            panic(&fst.loc, "TODO: `module` stmt not implemented");
            alf->status = ALF_STATUS_FCK;
            break; 
        } case TK_IMPORT: {
            panic(&fst.loc, "TODO: `import` stmt not implemented");
            alf->status = ALF_STATUS_FCK;
            break;
        } default: {
            panic(&fst.loc,
                  "Expected module | import | function definition."
                  "Provided token:%s:\""StrFmt"\" for unknown"
                  "primary statement", token_type_as_cstr(fst.type),
                  StrArgs(fst.text));
            defer: {
                alf->status = ALF_STATUS_FCK;
                return;
            }
        }
    }
}

static void parse_module(Module *m, Lexer *l) {
    for (;;) {
        parse_prime_statement(m, l);
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
        panicf(ErrFmt, "Could not find entry point."
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
    load_builtins(&m->a, a);
    parse_module(m, &l);
    alf_badcase(a); // Bad parsing?
    entry_point(a, m);
    alf_badcase(a); // No entry?
    vm_run_program(a);
    defer: { // Let me free!
        unload_builtins(a);
        map_free(&m->callstack);
        map_free(&m->typetable);
        map_free(&l.keywords);
        arena_free(&m->a);
    }
}
