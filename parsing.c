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

/* Definindo códigos para os erros possíveis. */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Definindo tipos de valores possíveis. */
enum { LVAL_NUM, LVAL_ERR };

/* Definindo o novo tipo lval */
typedef struct {
    int type;
    long num;
    int err;
} lval;

/* Função para criar um novo lval numérico */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* Função para criar um novo lval de erro */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* Printar um lval */
void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM: printf("%li", v.num); break;
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) { printf("Erro: Divisao por 0!"); }
            if (v.err == LERR_BAD_OP)   { printf("Erro: Operador invalido!"); }
            if (v.err == LERR_BAD_NUM)  { printf("Error: Número invalido!"); }
        break;
    }
}

/* Printar um lval com nova linha */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

/* Usando o operador String para ver qual operacao deve-se realizar */
lval eval_op(lval x, char* op, lval y) {
    
    /* Se o valor either é um erro, retorna ele */
    if(x.type == LVAL_ERR) { return x; }
    if(y.type == LVAL_ERR) { return y; }

    /* De outra maneira, realiza a operacao */
    if(strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if(strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if(strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if(strcmp(op, "/") == 0) { 
        /* Checa por divisao por 0 */
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    /* Se o nó é um número */
    if (strstr(t->tag, "number")) {
        /* Verifica se existe erro na conversão */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* O operador é o segundo filho */
    char* op = t->children[1]->contents;

    /* Avaliando o primeiro operando */
    lval x = eval(t->children[2]);

    /* Iterando sobre os outros operandos e aplicando o operador */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
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
            lval result = eval(r.output);
            lval_println(result);
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
