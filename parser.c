#include "assert.h"

#include "parser.h"


char * grammar ="                              \
number   : /-?[0-9]+/ ;                        \
string   : /\"(\\\\.|[^\"])*\"/ ;              \
symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&:]+/ ; \
comment  : /;[^\\r\\n]*/ ;                     \
sexpr    : '(' <expr>* ')' ;                   \
qexpr    : '{' <expr>* '}' ;                   \
expr     : <number> | <symbol> | <string>      \
  | <comment> | <sexpr> | <qexpr> ;            \
lispy    : /^/ <expr>* /$/ ;                   \
";

void init_parser() {
    Number   = mpc_new("number");
    String   = mpc_new("string");
    Symbol   = mpc_new("symbol");
    Comment  = mpc_new("comment");
    Sexpr    = mpc_new("sexpr");
    Qexpr    = mpc_new("qexpr");
    Expr     = mpc_new("expr");
    Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, grammar,
              Number, String, Symbol, Comment, Sexpr, Qexpr, Expr, Lispy);
}

void tear_down_parser() {
    mpc_cleanup(8, Number, String, Symbol, Comment, Sexpr, Qexpr, Expr, Lispy);
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
    }
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
    }
    if (strstr(t->tag, "string")) {
        t->contents[strlen(t->contents) - 1] = '\0';
        char* unescaped = malloc(strlen(t->contents+1)+1);

        strcpy(unescaped, t->contents+1);
        unescaped = mpcf_unescape(unescaped);
        lval* str = lval_str(unescaped);
        free(unescaped);
        return str;
    }
    if (strcmp(t->tag, ">") == 0 ||
        strstr(t->tag, "sexpr") ||
        strstr(t->tag, "qexpr"))
    {
        lval* ret = strstr(t->tag, "qexpr") ? lval_qexpr() : lval_sexpr();
        for(int i = 1; i < t->children_num - 1; ++i) {
            if (strstr(t->children[i]->tag, "comment")) {
                continue;
            }
            lval_add(ret, lval_read(t->children[i]));
        }
        return ret;
    }
    assert( 0 );
}
