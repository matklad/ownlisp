#ifndef EVAL_H
#define EVAL_H

#include "lval.h"

lval* lval_eval(lenv* e, lval* v);
lval* op_load(lenv *, lval* v);
void lenv_add_builtins(lenv* e);

#endif
