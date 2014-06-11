#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lval.h"
#include "mpc.h"

char* lval_type_name(int type){
    switch (type) {
    case LVAL_ERR: return "Error";
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_QEXPR: return "Q-Expression";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_STR: return "String";
    case LVAL_SYM: return "Symbol";
    default:
        assert( 0 );
    }
}

lval* _lval_new () {
    return calloc(1, sizeof(lval));
}

lval* lval_err(char *fmt, ...) {
    static const int buff_size = 512;
    lval* ret = _lval_new();
    ret->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);
    ret->err = malloc(buff_size);
    vsnprintf(ret->err, buff_size - 1, fmt, va);
    ret->err = realloc(ret->err, strlen(ret->err) + 1);
    va_end(va);

    return ret;
}

lval* lval_builtin(lbuiltin builtin) {
    lval* ret = _lval_new();
    ret->type = LVAL_FUN;
    ret->builtin = builtin;
    return ret;
}

lval* lval_lambda(lval* formals, lval* body) {
    lval* ret = _lval_new();
    ret->type = LVAL_FUN;
    ret->formals = formals;
    ret->body = body;
    ret->env = lenv_new();
    return ret;
}

lval* lval_num(long num) {
    lval* ret = _lval_new();
    ret->type = LVAL_NUM;
    ret->num = num;
    return ret;
}

lval* lval_qexpr(void) {
    lval* ret = _lval_new();
    ret->type = LVAL_QEXPR;
    ret->count = 0;
    ret->cell = NULL;
    return ret;
}

lval* lval_sexpr(void) {
    lval* ret = _lval_new();
    ret->type = LVAL_SEXPR;
    ret->count = 0;
    ret->cell = NULL;
    return ret;
}

lval* lval_str(char * str) {
    lval*ret = _lval_new();
    ret->type = LVAL_STR;
    ret->str = malloc(strlen(str) + 1);
    strcpy(ret->str, str);
    return ret;
}

lval* lval_sym(char* sym) {
    lval* ret = _lval_new();
    ret->type = LVAL_SYM;
    ret->sym = malloc(strlen(sym) + 1);
    strcpy(ret->sym, sym);
    return ret;
}

void lval_add(lval* v, lval* x){
    assert( v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
}

lval* lval_copy(lval* v) {
    lval* ret = _lval_new();
    memcpy(ret, v, sizeof(lval));

    switch (v->type) {
    case LVAL_ERR:
        ret->err = malloc(strlen(v->err) + 1);
        strcpy(ret->err, v->err);
        break;
    case LVAL_FUN:
        if(!v->builtin) {
            ret->env = lenv_copy(v->env);
            ret->formals = lval_copy(v->formals);
            ret->body = lval_copy(v->body);
        }
        break;
    case LVAL_NUM:
        break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        ret->cell = malloc(sizeof(lval*) * v->count);
        for (int i = 0; i < v->count; ++i) {
            ret->cell[i] = lval_copy(v->cell[i]);
        }
        break;
    case LVAL_STR:
        ret->str = malloc(strlen(v->str) + 1);
        strcpy(ret->sym, v->sym);
    case LVAL_SYM:
        ret->sym = malloc(strlen(v->sym) + 1);
        strcpy(ret->sym, v->sym);
        break;
    default:
        assert( 0 );
    }
    return ret;
}

void lval_del(lval* v) {
    switch (v->type) {
    case LVAL_ERR:
        free(v->err);
        break;
    case LVAL_FUN:
        if(!v->builtin) {
            lenv_del(v->env);
            lval_del(v->formals);
            lval_del(v->body);
        }
        break;
    case LVAL_NUM:
        break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        for (int i = 0; i < v->count; ++i) {
            lval_del(v->cell[i]);
        }
        free(v->cell);
        break;
    case LVAL_STR:
        free(v->str);
        break;
    case LVAL_SYM:
        free(v->sym);
        break;
    default:
        assert( 0 );
    }
    free(v);
}

void _lval_print_sexpr(lval* v, char open, char close) {
    putchar(open);
    if (v->count == 0) {
        putchar(close);
        return;
    }
    for (int i = 0; i < v->count - 1; ++i) {
        lval_print(v->cell[i]);
        putchar(' ');
    }
    lval_print(v->cell[v->count - 1]);
    putchar(close);
}

void lval_print(lval* v) {
    switch (v->type) {
    case LVAL_ERR:
        printf("Error:\n  %s", v->err);
        break;
    case LVAL_FUN:
        if(v->builtin) {
            printf("<builtin>");
        } else {
            printf("(\\");
            lval_print(v->formals);
            putchar(' ');
            lval_print(v->body);
            putchar(')');
        }
        break;
    case LVAL_NUM:
        printf("%li", v->num);
        break;
    case LVAL_QEXPR:
        _lval_print_sexpr(v, '{', '}');
        break;
    case LVAL_SEXPR:
        _lval_print_sexpr(v, '(', ')');
        break;
    case LVAL_STR:
        {
            char* escaped = malloc(strlen(v->str) + 1);
            strcpy(escaped, v->str);
            escaped = mpcf_escape(escaped);
            printf("\"%s\"", escaped);
            free(escaped);
            break;
        }
    case LVAL_SYM:
        printf("%s", v->sym);
        break;
    default:
        assert( 0 );
    }
}

void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

lenv* lenv_new() {
    lenv* ret = calloc(1, sizeof(lenv));
    return ret;
}

lenv* lenv_copy(lenv* e){
    lenv* ret = lenv_new();
    ret->parent = e->parent;
    ret->count = e->count;
    ret->syms = malloc(sizeof(char*) * ret->count);
    ret->vals = malloc(sizeof(lval*) * ret->count);
    for (int i = 0; i < ret->count; ++i) {
        ret->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(ret->syms[i], e->syms[i]);
        ret->vals[i] = lval_copy(e->vals[i]);
    }
    return ret;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; ++i) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
    return;
}

lval* lenv_get(lenv* e, lval* k) {
    assert (k->type == LVAL_SYM);
    for (int i = 0; i < e->count; ++i) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    if (e->parent) {
        return lenv_get(e->parent, k);
    }
    return lval_err("Unbound symbol %s!", k->sym);
}

void lenv_put(lenv*e, lval* k, lval* v) {
    assert (k->type == LVAL_SYM);
    for (int i = 0; i < e->count; ++i) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_def(lenv*e, lval* k, lval* v) {
    while (e->parent) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}
