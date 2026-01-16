#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

/* Se Windows */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Função readline para Windows */
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0'; // Remove o caractere de nova linha
    return cpy;
}

/* Função add_history para Windows (não faz nada) */
void add_history(char* unused) {}

/* Senão, se não for Windows, inclua as bibliotecas readline padrão */
#else
#include <editline/readline.h>
#endif

/* Usando o operador String para ver qual operacao deve-se realizar */
long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    return 0;
}

long eval(mpc_ast_t* t) {
    /* Se o nó é um número */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* O operador é o segundo filho */
    char* op = t->children[1]->contents;

    /* Avaliando o primeiro operando */
    long x = eval(t->children[2]);

    /* Iterando sobre os outros operandos e aplicando o operador */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        long y = eval(t->children[i]);
        x = eval_op(x, op, y);
        i++;
    }

    return x;
}

int main(int argc, char** argv) {

    /* Criando parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Circe = mpc_new("circe");


    /* Definindo a gramática */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
            number   : /-?[0-9]+/ ;                           \
            operator : '+' | '-' | '*' | '/' ;                \
            expr     : <number> | '(' <operator> <expr>+ ')' ;\
            circe    : /^/ <operator> <expr>+ /$/ ;           \
        ",
        Number, Operator, Expr, Circe);

    puts("Circe Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* input = readline("Circe> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Circe, &r)) {
            long result = eval(r.output);
            printf("%li\n", result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Liberando e deletando parsers */
        mpc_cleanup(4, Number, Operator, Expr, Circe);

        /* Liberando a memória alocada para a entrada */
        free(input);
    }

    return 0;
}
