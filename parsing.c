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
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v.err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')');
    }
}

/* Printar um lval com nova linha */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

/* Usando o operador String para ver qual operacao deve-se realizar */
lval* lval_pop(lval* v, int i) {
    
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

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

void lval_expr_print(lval v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v.count; i++) {
        lval_print(*v.cell[i]);
        if (i != (v.count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

lval* builtin_op(lval* a, char* op) {
    for(int i = 0; i < a->count; i++) {
        /* Verifica se todos os elementos são números */
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Operador não pode operar sobre tipos não números!");
        }
    }
    lval* x = lval_pop(a, 0);
    /* Se o operador é '-', e só tem um operando, faz a negação */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;           
    }

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Erro: Divisão por zero!");
                break;
            }
            x->num /= y->num;
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}   

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {
    /* Avaliando todas as células */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* Verificando por erros */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    /* Se a expressão está vazia, retornar ela */
    if (v->count == 0) {
        return v;
    }

    /* Se a expressão tem apenas um elemento, retornar esse elemento */
    if (v->count == 1) {
        return lval_take(v, 0);
    }

    /* Garantir que o primeiro elemento é um símbolo */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("Primeiro elemento não é um operador!");
    }

    /* Aplicar o operador */
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval* v) {
    /* Se o lval é uma expressão S */
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(v);
    }
    /* Senão, retornar o próprio lval */
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, 10, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("Número inválido!");
}

lval* lval_read(mpc_ast_t* t){
    /* Se o simbolo ou número retorna a conversao daquele tipo */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* Se for root (>) ou sexpr */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }

    /* Preenchendo essas lista com qualquer expressao valida */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));     
    }
}

int main(int argc, char** argv) {

    /* Criando parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Circe = mpc_new("circe");


    /* Definindo a gramática */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
            number   : /-?[0-9]+/ ;                           \
            operator : '+' | '-' | '*' | '/' ;                \
            expr     : <number> | <symbol> | <sexpr> ;\
            circe    : /^/ <expr>* /$/ ;           \
        ",
        Number, Symbol, Sexpr, Expr, Circe);

    puts("Circe Version 0.0.0.0.5");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* input = readline("Circe> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Circe, &r)) {
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        }else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Liberando a memória alocada para a entrada */
        free(input);
    }
    
    /* Liberando e deletando parsers */
    mpc_cleanup(4, Number, Operator, Expr, Circe);

    return 0;
}
