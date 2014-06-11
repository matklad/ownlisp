#ifndef PARSER_H
#define PARSER_H

#include "mpc.h"
#include "lval.h"

mpc_parser_t* Number;
mpc_parser_t* String;
mpc_parser_t* Symbol;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

void init_parser();

void tear_down_parser();

lval* lval_read(mpc_ast_t* t);

#endif
