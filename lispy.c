#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "lval.h"
#include "eval.h"
#include "parser.h"

int main(int argc, char** argv) {
    puts("Lispy version 0.0.0.4");
    puts("Press Ctrl-c to Exit\n");

    init_parser();

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            lval* args = lval_sexpr();
            lval_add(args, lval_str(argv[i]));
            lval* x = op_load(e, args);
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
    }
    while (1) {
        char * input = readline("> ");
        add_history(input);
        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* in = lval_read(r.output);

            lval* res  = lval_eval(e, in);
            lval_println(res);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }
    lenv_del(e);
    tear_down_parser();
    return 0;
}
