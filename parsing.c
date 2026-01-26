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
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR};

/* Definindo o novo tipo lval */
typedef struct lval {
    int type;
    long num;

    /* Erro e símbolo são representados como dados string */
    char err;
    char sym;

    /* Contador de células e ponteiro para células */
    int count;
    struct lval** cell;

} lval;

/* Construir um ponteiro par um novo Número lval */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Função para criar um ponteiro para novo lval de erro */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* Função para criar um ponteiro para novo lval de símbolo */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* Função para criar um ponteiro para novo lval de expressão S */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval v) {
    switch (v->type) {
        /* Nada especial para números */
        case LVAL_NUM: break;

        /* Para erros e símbolos, liberar a memória alocada para as strings */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* Para expressões S, liberar todas as células */
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
        break;
    }

    /* Finalmente, liberar o próprio lval */
    free(v);
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
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
lval eval_op(lval* v, int i) {
    
    /* Pegando o item em i */
    lval* x = v->cell[i];

    /* Alterando a memória depois do item i para acima do topo */
    memmove(&v->cell[i], &v->cell[i+1],
        sizeof(lval*) * (v->count - i - 1));
    
    /* Decrementando o contador de células */
    v->count--;
    
    /* Realocando a memória usada */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
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
