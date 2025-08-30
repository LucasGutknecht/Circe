#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

// Proximos passos
// 1. Adicionar os itens de preprocessador.
// 2. Adicionar os Parser Combinators

int main(int argc, char** argv) {
    puts("Circe Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* input = readline("Circe> ");
        add_history(input);
        printf("No you're a %s\n", input);
        free(input);
    }

    return 0;
}
