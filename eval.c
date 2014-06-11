#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "lval.h"
#include "parser.h"

lval* _lval_pop(lval* v, int i) {
    lval* ret = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - i - 1));

    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return ret;
}

void _lval_popd(lval* v, int i) {
    lval_del(_lval_pop(v, i));
}

lval* _lval_take(lval* v, int i) {
    lval* ret = _lval_pop(v, i);
    lval_del(v);
    return ret;
}

#define LASSERT(arg, cond, fmt, ...)              \
    if (!(cond)) {                                \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(arg) ;                           \
        return err;                               \
    }

#define LASSERT_NUM(arg, num, name)                                     \
    LASSERT(arg, arg->count == num,                                     \
            name ": expected %i arguments, got %i!", num, arg->count)

#define LASSERT_TYPE(arg, type, expected, name) \
    LASSERT(arg, type == expected,              \
            name ": expected %s got %s!",       \
            lval_type_name(expected),          \
            lval_type_name(type))


lval* _op_head(lenv* e, lval* v) {
    LASSERT_NUM(v, 1, "head");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "head");
    LASSERT(v, v->cell[0]->count > 0, "head: empty list!");
    lval* ret= _lval_take(v, 0);
    while(ret->count > 1) {
        _lval_popd(ret, 1);
    }
    return ret;
}

lval* _op_tail(lenv* e, lval* v) {
    LASSERT_NUM(v, 1, "tail");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "tail");
    LASSERT(v, v->cell[0]->count > 0, "tail: empty list!");

    lval* ret = _lval_take(v, 0);
    _lval_popd(ret, 0);
    return ret;
}

lval* _op_list(lenv* e, lval* v) {
    v->type = LVAL_QEXPR;
    return v;
}

lval* _op_eval(lenv* e, lval* v) {
    LASSERT_NUM(v, 1, "eval");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "eval");

    lval* x = _lval_take(v, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* _op_join(lenv* e, lval* v) {
    for(int i = 0; i < v->count; ++i) {
        LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "join");
    }
    lval* ret = lval_qexpr();
    while(v->count > 0) {
        lval* x = _lval_pop(v, 0);
        while(x->count > 0) {
            lval_add(ret, _lval_pop(x, 0));
        }
        lval_del(x);
    }
    lval_del(v);
    return ret;
}

lval* _op_arith(lenv* e, lval* v, char* op) {
    for(int i = 0; i < v->count; ++i) {
        LASSERT_TYPE(v, v->cell[i]->type, LVAL_NUM, "operator");
    }
    LASSERT(v, (v->count > 0 || (strcmp(op, "-") != 0 && strcmp(op, "/") != 0)),
            "Operator (%s): wrong arity", op);

    lval* x =  _lval_pop(v, 0);

    if (strcmp(op, "-") == 0 && v->count == 0) {
        x->num = -x->num;
    }

    while (v->count > 0) {
        lval* y = _lval_pop(v, 0);
        if (strcmp(op, "+") == 0)
            x->num += y->num;

        else if (strcmp(op, "-") == 0)
            x->num -= y->num;

        else if (strcmp(op, "*") == 0)
            x->num *= y->num;

        else if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                lval_del(v);
                return lval_err("Division by zero!");
            }
            x->num /= y->num;
        } else {
            assert( 0 );
        }
        lval_del(y);
    }
    lval_del(v);
    return x;
}

lval* _op_add(lenv* e, lval* v) { return _op_arith(e, v, "+"); }
lval* _op_sub(lenv* e, lval* v) { return _op_arith(e, v, "-"); }
lval* _op_mul(lenv* e, lval* v) { return _op_arith(e, v, "*"); }
lval* _op_div(lenv* e, lval* v) { return _op_arith(e, v, "/"); }

lval* _op_cmp(lenv* e, lval* v, char* op) {
    LASSERT_NUM(v, 2, "comparison");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_NUM, "comparison");
    LASSERT_TYPE(v, v->cell[1]->type, LVAL_NUM, "comparison");
    int x = v->cell[0]->num;
    int y = v->cell[1]->num;
    lval_del(v);
    int r;
    if (strcmp(op, "<") == 0) {
        r = x < y;
    }
    if (strcmp(op, "<=") == 0) {
        r = x <= y;
    }
    if (strcmp(op, ">") == 0) {
        r = x > y;
    }
    if (strcmp(op, ">=") == 0) {
        r = x >= y;
    }

    return lval_num(r);
}

lval* _op_lt(lenv* e, lval* v) { return _op_cmp(e, v, "<"); }
lval* _op_le(lenv* e, lval* v) { return _op_cmp(e, v, "<="); }
lval* _op_gt(lenv* e, lval* v) { return _op_cmp(e, v, ">"); }
lval* _op_ge(lenv* e, lval* v) { return _op_cmp(e, v, ">="); }

int _lval_equals(lval* a, lval* b) {
    if (a->type != b->type) {
        return 0;
    }
    switch (a->type) {
    case LVAL_ERR:
        return strcmp(a->err, b->err) == 0;
    case LVAL_FUN:
        if (a->builtin || b->builtin) {
            return a->builtin == b->builtin;
        }
        return _lval_equals(a->formals, b->formals) &&  \
            _lval_equals(a->body, b->body);
    case LVAL_NUM:
        return a->num == b->num;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        if (a->count != b->count) {
            return 0;
        }
        int ret = 1;
        for (int i = 0; i < a->count; ++i) {
            ret |= _lval_equals(a->cell[i], b->cell[i]);
        }
        return ret;
    case LVAL_STR:
        return strcmp(a->str, b->str);
    case LVAL_SYM:
        return strcmp(a->sym, b->sym) == 0;
    default:
        assert( 0 );
    }
}

lval* _lval_op_equality(lenv* e, lval* v, char* op) {
    LASSERT_NUM(v, 2, "==");
    lval* a = v->cell[0];
    lval* b = v->cell[1];
    int eq = _lval_equals(a, b);
    lval_del(v);
    return lval_num(strcmp(op, "==") == 0 ? eq : !eq);
}

lval* _op_eq(lenv* e, lval* v) { return _lval_op_equality(e, v, "=="); }
lval* _op_neq(lenv* e, lval* v) { return _lval_op_equality(e, v, "/="); }

lval* _op_if(lenv* e, lval* v) {
    LASSERT_NUM(v, 3, "if");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_NUM, "if");
    LASSERT_TYPE(v, v->cell[1]->type, LVAL_QEXPR, "if");
    LASSERT_TYPE(v, v->cell[2]->type, LVAL_QEXPR, "if");
    lval* cond_val = _lval_pop(v, 0);
    int cond = cond_val->num;
    lval_del(cond_val);
    lval* a = _lval_pop(v, 0);
    lval* b = _lval_take(v, 0);
    if (!cond) {
        lval* t = a;
        a = b;
        b = t;
    }
    lval_del(b);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}

lval* _op_definition(lenv* e, lval* v, char* op) {
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "def");
    lval* syms = v->cell[0];
    for (int i = 0; i < syms->count; ++i) {
        LASSERT_TYPE(v, syms->cell[i]->type, LVAL_SYM, "def");
    }
    LASSERT_NUM(v, syms->count + 1, "def");

    for (int i = 0; i < syms->count; ++i) {
        if (strcmp(op, "def") == 0) {
            lenv_def(e, syms->cell[i], v->cell[i + 1]);
        } else {
            assert( strcmp(op, ":=") == 0 );
            lenv_put(e, syms->cell[i], v->cell[i + 1]);
        }
    }
    lval_del(v);
    return lval_sexpr();
}

lval* _op_def(lenv* e, lval* v) { return _op_definition(e, v, "def"); }
lval* _op_assign(lenv* e, lval* v) { return _op_definition(e, v, ":="); }

lval* _op_lambda(lenv* e, lval* v) {
    LASSERT_NUM(v, 2, "\\");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_QEXPR, "\\");
    LASSERT_TYPE(v, v->cell[1]->type, LVAL_QEXPR, "\\");
    for (int i = 0; i < v->cell[0]->count; ++i) {
        LASSERT_TYPE(v, v->cell[0]->cell[i]->type, LVAL_SYM, "\\");
    }

    lval* formals = _lval_pop(v, 0);
    lval* body = _lval_take(v, 0);

    lval* ret = lval_lambda(formals, body);

    return ret;
}

lval* _op_print(lenv* e, lval* v) {
    for(int i = 0; i < v->count - 1; ++i) {
        lval_print(v->cell[i]);
        putchar(' ');
    }
    lval_println(v->cell[v->count - 1]);
    lval_del(v);
    return lval_sexpr();
}

lval* _op_error(lenv* e, lval* v) {
    LASSERT_NUM(v, 1, "error");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_STR, "error");
    lval* ret = lval_err(v->cell[0]->str);
    lval_del(v);
    return ret;
}

lval* op_load(lenv* e, lval* v) {
    LASSERT_NUM(v, 1, "load");
    LASSERT_TYPE(v, v->cell[0]->type, LVAL_STR, "load");

    mpc_result_t r;
    if (!mpc_parse_contents(v->cell[0]->str, Lispy, &r)) {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);
        lval* err = lval_err("load: failed to load \"%s\": %s",
                             v->cell[0]->str, err_msg);
        free(err_msg);
        lval_del(v);
        return err;
    } else {
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);
        while (expr->count) {
            lval* x = lval_eval(e, _lval_pop(expr, 0));
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
        lval_del(expr);
        lval_del(v);

        return lval_sexpr();
    }
}

void lenv_add_builtin(lenv* e, lbuiltin builtin, char* name) {
    lval* k = lval_sym(name);
    lval* v = lval_builtin(builtin);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, &_op_add, "+");
    lenv_add_builtin(e, &_op_sub, "-");
    lenv_add_builtin(e, &_op_mul, "*");
    lenv_add_builtin(e, &_op_div, "/");

    lenv_add_builtin(e, &_op_lt, "<");
    lenv_add_builtin(e, &_op_le, "<=");
    lenv_add_builtin(e, &_op_gt, ">");
    lenv_add_builtin(e, &_op_ge, ">=");

    lenv_add_builtin(e, &_op_eq, "==");
    lenv_add_builtin(e, &_op_neq, "/=");

    lenv_add_builtin(e, &_op_head, "head");
    lenv_add_builtin(e, &_op_tail, "tail");
    lenv_add_builtin(e, &_op_list, "list");
    lenv_add_builtin(e, &_op_join, "join");
    lenv_add_builtin(e, &_op_eval, "eval");

    lenv_add_builtin(e, &_op_if, "if");

    lenv_add_builtin(e, &_op_def, "def");
    lenv_add_builtin(e, &_op_assign, ":=");

    lenv_add_builtin(e, &_op_lambda, "\\");

    lenv_add_builtin(e, &op_load, "load");

    lenv_add_builtin(e, &_op_print, "print");
    lenv_add_builtin(e, &_op_error, "error");
}

lval* _lval_call(lenv* e, lval* f, lval* v) {
    if (f->builtin) {
        return f->builtin(e, v);
    }

    int n_formals = f->formals->count;
    int n_actual = v->count;
    while (v->count) {
        if (!f->formals->count) {
            lval_del(v);
            return lval_err("Too many arguments, expected %i, got %i!",
                            n_formals, n_actual);
        }
        lval* formal = _lval_pop(f->formals, 0);
        if (strcmp(formal->sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(v);
                return lval_err("Bad varargs!");
            }
            lval* v_formal = _lval_pop(f->formals, 0);
            lenv_put(e, v_formal, _op_list(e, v));
            break;
        }
        lval* actual = _lval_pop(v, 0);
        lenv_put(f->env, formal, actual);
        lval_del(formal);
        lval_del(actual);
    }
    lval_del(v);
    if (f->formals->count == 0) {
        f->env->parent = e;
        f->body->type = LVAL_SEXPR;
        return lval_eval(f->env, lval_copy(f->body));
    }
    if (strcmp(f->formals->cell[0]->sym, "&") == 0) {
        if (f->formals->count != 2) {
            return lval_err("Bad varargs");
        }
        _lval_popd(f->formals, 0);
        lval* v_formal = _lval_pop(f->formals, 0);
        lval* v_actual = lval_qexpr();
        lenv_put(e, v_formal, v_actual);
        lval_del(v_formal);
        lval_del(v_actual);
    }
    return lval_copy(f);
}

lval* _lval_eval_sexp(lenv* e, lval* v) {
    assert( v->type == LVAL_SEXPR );
    for(int i = 0; i < v->count; ++i) {
        v->cell[i] = lval_eval(e, v->cell[i]);
        if ( v->cell[i]->type == LVAL_ERR) {
            return _lval_take(v, i);
        }
    }

    if (v->count == 0) {
        return v;
    }

    if (v->count == 1) {
        return _lval_take(v, 0);
    }

    lval* f = _lval_pop(v, 0);
    if(f->type != LVAL_FUN) {
        lval* ret = lval_err("First element is not a function (%s)!",
                             lval_type_name(f->type));
        lval_del(f);
        lval_del(v);
        return ret;
    }

    lval* ret = _lval_call(e, f, v);
    lval_del(f);
    return ret;
}

lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        lval* ret = lenv_get(e, v);
        lval_del(v);
        return ret;
    }
    lval* ret = v->type == LVAL_SEXPR ? _lval_eval_sexp(e, v) : v;
    return ret;
}
