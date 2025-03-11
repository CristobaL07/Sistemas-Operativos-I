#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define _POSIX_C_SOURCE_200112L
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define PROMPT '$'
#define RESET "\033[0m"
#define GRIS_T "\x1b[90m"

char *read_line(char *line);
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);


int main() {
    char line[COMMAND_LINE_SIZE];
    //bucle infinito para leer las 
    //líneas y ejecutarlas
    while (1) {
        if (read_line(line)) {
        execute_line(line);
        }
    }
    return 0;
}

//esta función imprime el prompt y lee la línea pasada por 
//teclado y lo almacena en una variable line que será lo 
// que devuelva la función
char *read_line(char *line) {
    printf("%c ", PROMPT);
    fflush(stdout);
    if(fgets(line, COMMAND_LINE_SIZE, stdin)) {
        if (line[strlen(line)-1] == '\n') {
            line[strlen(line)-1] = '\0';
        }
    }
    else {
        exit(0);
    }
    return (char*) line;
}

int execute_line(char *line) {
    char *args[ARGS_SIZE];
    //troceamos la línea pasada por
    //parámetro en tokens y lo guardamos 
    //en args
    parse_args(args,line);
    //comprobamos si se 
    //trata de un proceso interno
    check_internal(args);
}

//troceamos la línea pasada por teclado en tokens y lo almacenamos en args
int parse_args(char **args, char *line) {
    char *token;
    char s[4] = " \t\n\r";
    int num = 0;
    token = strtok(line,s);
    printf(GRIS_T "[parse_args()->token %d: %s]\n" RESET, num, token);  
    args[num] = token;
    num++;
    while(token != NULL) {
        if(token[0] == '#') {
            --num;
            token = NULL;
            printf(GRIS_T "[parse_args()->token %d corregido: %s]\n" RESET, num, token);
        }
        else {
            token = strtok(NULL,s);
            printf(GRIS_T "[parse_args()->token %d: %s]\n" RESET, num, token);
            args[num] = token;
            num++;
        }
    }
    return num;
}

//comprobamos si se trata de un comando interno
//y, si es así, miramos de qué tipo se trata
int check_internal(char **args) {
    if(args[0] != NULL){ 
        int cmp = strcmp(args[0], "cd");
        if(cmp == 0) {
            internal_cd(args);
        }
        cmp = strcmp(args[0], "export");
        if(cmp == 0) {
            internal_export(args);
        }
        cmp = strcmp(args[0], "source");
        if(cmp == 0) {
            internal_source(args);
        }
        cmp = strcmp(args[0], "jobs");
        if(cmp == 0) {
            internal_jobs(args);
        }
        cmp = strcmp(args[0], "fg");
        if(cmp == 0) {
            internal_fg(args);
        }
        cmp = strcmp(args[0], "bg");
        if(cmp == 0) {
            internal_bg(args);
        }
        cmp = strcmp(args[0], "exit");
        if(cmp == 0) {
            exit(0);
        }
    }
}

int internal_cd(char **args) {
    printf(GRIS_T "[internal_cd()->Esta función cambiará de directorio]\n" RESET);
}

int internal_export(char **args) {
    printf(GRIS_T "[internal_export()->Esta función asignará valores a variablescd de entorno]\n" RESET);
}

int internal_source(char **args) {
    printf(GRIS_T "[internal_source()->Esta función ejecutará un fichero de líneas de comandos]\n" RESET);
}

int internal_jobs(char **args) {
    printf(GRIS_T "[internal_jobs()->Esta función mostrará el PID de los procesos que no esten en foreground]\n" RESET);
}

int internal_fg(char **args) {
    
}

int internal_bg(char **args) {
    
}