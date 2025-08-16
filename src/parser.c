#include <assert.h>
#include "parser.h"
#include "lexer.h"

/*
 *      Utils
 */

const char *builtin_types[BUILTIN_TYPES_COUNT] = {
    "void", "int", "uint", "flt", "bool", "char"
};

ALF_FUNC alf_uint *typeget(Map *t, CString *key) {
    alf_uint *id;
    map_strget(t, key->items, key->count, id);
    return id;
}

ALF_FUNC int checktype(Map *m, CString *name) {
    alf_uint *id;
    map_strget(m, name->items, name->count, id);
    if (!id) return 0;
    else return 1;
}

ALF_FUNC Context *context_init(Module *m) {
    Context *c = arena_alloc(&m->a, sizeof(Context));
    c->codesize = c->datasize = 0;
    c->codeptr = c->dataptr = 0;
    c->code = NULL;
    c->data = NULL;
    c->module = m;
    return c;
}

ALF_FUNC Fundef *fundef_search(Module *m, CString *name) {
    Fundef *f = NULL;
    map_strget(&m->callstack, name->items, name->count, f);
    return f;
}

void cntx_pushinst(Arena *a, Context *c, Opcode o) {
    arena_da_append_parts(a, c->code, c->codeptr, c->codesize, o);
}

void cntx_pushdata(Arena *a, Context *c, StkVal v) {
    arena_da_append_parts(a, c->data, c->dataptr, c->datasize, v);
}

/*
 *      Core
 */ 

ALF_FUNC StkVal vm_popdata(Context *c) {
    assert(c->dataptr != 0 && "Could not pop empty data value");
    StkVal result = c->data[--c->dataptr];
    return result;
}

ALF_FUNC Opcode vm_popinst(Context *c) {
    assert(c->codeptr != 0 && "Could not pop empty instruction");
    Instruction result = c->code[--c->codeptr];
    return instext(result);
}

typedef void (bltinfn)(Alf_State *a, Context *c);

void putstr(Alf_State *a, Context *c) {
    String *s = vm_popdata(c).s;
    char *p = s->items;
    while (*p) putchar(*p++);
}

ALF_FUNC void vm_instexec(Alf_State *alf, Context *c, alf_byte mode) {
        Opcode op = vm_popinst(c);
        switch (op) {
            case OP_CALL: {
                break;
            }
            case OP_BLTIN: {
                Address a = vm_popdata(c).u;
                if (a == 0) putstr(alf, c);
                else {
                    alf_panic("TODO", "Not implemented other built-in functions");
                    alf->status = ALF_STATUS_FCK;
                }
                break;
            }
            default: {
                alf_panic("TODO:", "Not supported rest of instructions");
                alf->status = ALF_STATUS_FCK;
                return;
            }
        }
}

ALF_FUNC void vm_pushfunc(Module *m, Fundef *f) {
    (void)m; (void)f;
    alf_panic(Err_Fmt, "Could not store non-entry function call");
}

ALF_FUNC void vm_load_func(Alf_State *a, Fundef *f) {

}

ALF_FUNC void vm_run_program(Alf_State *a) {

}

/*
 *      Expression
 */

ALF_FUNC void vardeclare(Lexer *l, Token *type, Token *name) {
    (void)name;
    l->alf->status = ALF_STATUS_FCK;
    panic(&type->loc, "TODO: variable declararion not implemented");
    return;
}

ALF_FUNC void variable(Lexer *l, Token *type, Token *name) {
    vardeclare(l, type, name);
    return;
}

ALF_FUNC Funcall *funcall(Arena *a, Lexer *l, CString *name) {
    Funcall *f = arena_alloc(a, sizeof(Funcall));
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
        arena_da_append(a, f, arg);
    }
    lexer_expect(l, TK_SEMICOLON);
    return f;
}

Expr expr(Arena *a, Lexer *l) {
    Expr e = {0};
    Token tk = lexer_next(l);
    switch (tk.type) {
        case TK_STRLIT: {
            e.t = EXPR_STR;
            e.s = string(a, (char*)tk.text.items, tk.text.count);
            break;
        }
        case TK_ID: {
            e.t = EXPR_FUNCALL;
            e.fc = funcall(a, l, &tk.text);
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

ALF_FUNC void parse_context(Context *c, Lexer *l) {
    for (;;) {
        parse_statement(c, l);
        if (lexer_failer(l)) return;
        Token tk = lexer_peek(l);
        if (tk.type == TK_EOF
        ||  tk.type == TK_CCURLY) break;
    }
}

// TODO: Function declaration
ALF_FUNC void fundef(Module *m, Lexer *l, Token *t, CString *name) {
    Fundef f = {0};
    Alf_State *alf = l->alf;
    Map *typetable = &m->typetable;
    alf_uint *type = typeget(typetable, &t->text);
    if (type == NULL) {
        alf->status = ALF_STATUS_FCK;
        panic(&t->loc,
              "Unknown type `"Str_Fmt"`", 
              Str_Args(t->text));
        return;
    } 
    f.type = *type;
    lexer_expect(l, TK_OPAREN);
    if (lexer_failer(l)) return;
    // TODO: Parse arguments
    lexer_expect(l, TK_CPAREN);
    if (lexer_failer(l)) return;
    Context *body = context_init(m);
    f.c = body;
    lexer_expect(l, TK_OCURLY);
    if (lexer_failer(l)) return;
    parse_context(body, l);
    mapins(&m->callstack, name->items,
           name->count, &f);
    lexer_expect(l, TK_CCURLY);
}

ALF_FUNC void parse_expr(Context *c, Lexer *l) {
    Module *m = c->module;
    Alf_State *alf = l->alf;
    Token tk = lexer_peek(l);
    Expr e = expr(&m->a, l);
    switch (e.t) {
        case EXPR_FUNCALL: {
            Funcall *fc = e.fc;
            Fundef *f = fundef_search(m, &fc->name);
            if (!f) {
                alf->status = ALF_STATUS_FCK;
                alf_panic(Err_Fmt, "Call to not define function `"Str_Fmt"`",
                          Str_Args(fc->name));
                return;
            }
            if (!f->bltin) {
                cntx_pushinst(&m->a, c, OP_CALL);
                if (!f->used) { f->used = 1; vm_pushfunc(m, f); }
            }
            else cntx_pushinst(&m->a, c, OP_BLTIN);
            cntx_pushdata(&m->a, c, StkUint(f->addr));
            for (size_t i = 0; i < fc->count; ++i) {
                Expr arg = fc->items[i];
                switch (arg.t) {
                    case EXPR_STR: {
                        cntx_pushdata(&m->a, c, StkStr(arg.s));
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
        case TK_IF: {
            panic(&fst.loc, "TODO: `if` stmt not implemented");
            break; 
        }
        case TK_WHILE: {
            panic(&fst.loc, "TODO: `while` stmt not implemented");
            break; 
        }
        case TK_ID: {
            parse_expr(c, l); // Parse single expression
            break;
        }
        case TK_RETURN: {
            panic(&fst.loc, "TODO: `return` stmt not implemented");
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
            alf->status = ALF_STATUS_FCK;
            panic(&fst.loc, "Unknown statement started"
                  "from token `"Str_Fmt"`", fst.text);
            return;
        }
    }
}

ALF_FUNC void parse_primary_stmt(Module *m, Lexer *l) {
    Alf_State *alf = l->alf;
    Token fst = lexer_peek(l);
    switch (fst.type) {
        case TK_INT:  case TK_FLT:  case TK_BOOL:
        case TK_UINT: case TK_CHAR: case TK_VOID: case TK_ID: {
            if (checktype(&m->typetable, &fst.text)) {
                lexer_next(l); // Skip type
                Token name = lexer_expect(l, TK_ID);
                if (lexer_failer(l)) {
                    panic(&name.loc,
                          "Expected for variable or function"
                          "identifier, but proivded '%s'",
                          token_type_as_cstr(name.type));
                    alf->status = ALF_STATUS_FCK;
                    return;
                }
                fundef(m, l, &fst, &name.text);
            } else {
                alf->status = ALF_STATUS_FCK;
                panic(&fst.loc, "function declaration");
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
            panic(&fst.loc, "Unknown statement started from token `"Str_Fmt"`",
                          fst.text);
            alf->status = ALF_STATUS_FCK;
            return;
        }
    }

}

ALF_FUNC void parse_module(Module *m, Lexer *l) {
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

ALF_FUNC Module *global_module_init(void) {
    Arena a = {0};
    Module *m = arena_alloc(&a, sizeof(Module));
    Fundef *putstr = arena_alloc(&a, sizeof(Fundef));
    putstr->bltin = 1;
    mapins(&m->callstack, "putstr", 6, putstr);
    map_init(&m->typetable, BUILTIN_TYPES_COUNT*3);
    for (size_t i = 0; i < BUILTIN_TYPES_COUNT; ++i)
        map_ins(&m->typetable, builtin_types[i], &i);
    m->entry = 1;
    m->a = a;
    return m;
}


void mainfunc(Alf_State *alf, Reader *r) {
    Lexer l = lexer_init(alf, r);
    Module *m = global_module_init();
    parse_module(m, &l);
    if (alf->status) goto defer;
    CString entry_func_name = (CString)slice_parts("main", 4);
    Fundef *entry_func = fundef_search(m, &entry_func_name);
    if (!entry_func) {
        alf->status = ALF_STATUS_FCK;
        alf_panic(Err_Fmt, "Could not find entry point."
                           "Expected declaration of main function");
        goto defer;
    }
    vm_load_func(alf, entry_func);
    vm_run_program(alf);
    defer:
        map_free(&m->typetable);
        map_free(&l.keywords);
        arena_free(&m->a);
}
