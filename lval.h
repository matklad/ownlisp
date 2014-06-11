#ifndef LVAL_H
#define LVAL_H

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);

enum { LVAL_ERR,
       LVAL_FUN,
       LVAL_NUM,
       LVAL_QEXPR,
       LVAL_SEXPR,
       LVAL_STR,
       LVAL_SYM };

struct lval {
    int type;

    long num;

    int count;
    struct lval** cell;

    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    char* str;
    char* sym;
    char* err;
};

char* lval_type_name(int type);

lval* lval_err(char *err, ...);
lval* lval_builtin(lbuiltin builtin);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_num(long num);
lval* lval_qexpr(void);
lval* lval_sexpr(void);
lval* lval_str(char* str);
lval* lval_sym(char* sym);

void lval_add(lval* v, lval* x);

lval* lval_copy(lval* v);
void lval_del(lval* v);
void lval_print(lval* v);
void lval_println(lval* v);

struct lenv {
    lenv* parent;
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void);
lenv* lenv_copy(lenv* e);
void lenv_del(lenv* e);

lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv*e, lval* k, lval* v);
void lenv_def(lenv*e, lval* k, lval* v);

#endif
