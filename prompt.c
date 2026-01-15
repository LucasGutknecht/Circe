#include <stdio.h>
#include <stdlib.h>

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
