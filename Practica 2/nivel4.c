#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _POSIX_C_SOURCE_200112L
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define PROMPT '$'
#define RESET "\033[0m"
#define GRIS_T "\x1b[90m"
#define ROJO_T "\x1b[31m"
#define N_JOBS 64

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
void reaper(int signum);
void ctrlc(int signum);


struct info_job {
   pid_t pid;
   char status; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[COMMAND_LINE_SIZE]; // línea de comando asociada
};
static struct info_job jobs_list[N_JOBS];

static char mi_shell[COMMAND_LINE_SIZE];

int main (int argc, char *argv[]) {
    //guardamos el comando de ejecución
    //en una variable de tipo char
    if(argc==2){
        int cmp=strcmp(argv[0],"./nivel4");
        if(cmp==0){
            strcpy(mi_shell,argv[0]);
        }
    }
    //inicializamos los valores de jobs_list[0] (PID, estado y nombre)
    jobs_list[0].pid=0;
    jobs_list[0].status='N';
    
    for(int i=0;i<N_JOBS;i++){
        jobs_list[0].cmd[i]='\0';
    }
    //asignamos las señales del reaper, el ctrlc
    signal(SIGCHLD,reaper);
    signal(SIGINT,ctrlc);

    char line[COMMAND_LINE_SIZE];
    //bucle infinito para leer las 
    //líneas y ejecutarlas
    while(1){
        if(read_line(line)){
            execute_line(line);
        }
    }
    return 0;
}

//esta función imprime el prompt y lee la línea pasada por 
//teclado y lo almacena en una variable line que será lo 
// que devuelva la función
char *read_line(char *line){
    printf("%s %c ",mi_shell,PROMPT);
    fflush(stdout);
    fgets(line,1023,stdin);
    if(feof(stdin)){
        exit(0);
    }
    line[strlen(line)-1]='\0';
    return (char*) line;
}

//en el reaper maneja la señal SIGCHLD, la
//cual se activa por defecto. Una vez se activa, 
//el reaper busca el PID del proceso finalizado.
//Con el PID buscamos el proceso que ya ha finalizado.
//Una vez encontrado, si se encuentra 
//en primer plano, resetea los valores del mismo.
//En caso contrario, se elimina el proceso.
void reaper(int signum){
    char mensaje[1200];
    signal(SIGCHLD,reaper);
    while((jobs_list[0].pid = waitpid(-1,&jobs_list[0].status,WNOHANG))>0){
        sprintf(mensaje,"[reaper()->proceso hijo: %d (%s)finalizado con exit code %d]\n",jobs_list[0].pid, jobs_list[0].cmd,
        WEXITSTATUS(jobs_list[0].status));
        write(2,mensaje,strlen(mensaje));
        jobs_list[0].pid=0;
        jobs_list[0].status='F';
        for(int i=0;i<N_JOBS;i++){
            jobs_list[0].cmd[i]='\0';
        }
    }
}

//Finaliza el proceso que se ecuentra en primer plano.
//Este proceso maneja la señal SIGINT, la cual se activa 
//en caso de usar ctrl+c.
void ctrlc(int signum){
    char mensaje[1200];
    signal(SIGINT,ctrlc);
    sprintf(mensaje,"\n[ctrlc()-> Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),mi_shell,jobs_list[0].pid, jobs_list[0].cmd);
    write(2,mensaje,strlen(mensaje));
    if(jobs_list[0].pid>0){
        if(jobs_list[0].cmd!=mi_shell){
            sprintf(mensaje,"[ctrlc()->señal SIGTERM enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid, jobs_list[0].cmd,getpid(),mi_shell);
            write(2,mensaje,strlen(mensaje));
            kill(jobs_list[0].pid,SIGTERM);
        }else{
            sprintf(mensaje,"[ctrlc()->señal SIGTERM no enviado por %d (%s) debido a que su proceso en foreground es el shell]\n",jobs_list[0].pid, 
            jobs_list[0].cmd);
            write(2,mensaje,strlen(mensaje));
        }
    }else{
        sprintf(mensaje,"[ctrlc()->señal SIGTERM no enviado por %d (%s) debido a que no hay proceso en foreground]\n",jobs_list[0].pid, jobs_list[0].cmd);
        write(2,mensaje,strlen(mensaje));
        printf("%s %c ",mi_shell,PROMPT);
    }
    fflush(stdout);
}

int execute_line(char *line){
    char line2[COMMAND_LINE_SIZE];
    strcpy(line2,line);
    char *args[ARGS_SIZE];
    //troceamos la línea pasada por
    //parámetro en tokens y lo guardamos 
    //en args
    parse_args(args,line);
    //condicional que comprueba si se 
    //trata de un proceso interno
    if(check_internal(args)==0){
        //creamos un proceso hijo 
        //del mini shell 
        jobs_list[0].pid = fork();
        //a través de este condicional
        //comprobamos si se trata de un proceso 
        //hijo
        if(jobs_list[0].pid==0){
            //declaramos las distintas señales
            //para el reaper, el ctrlc
            //respectivamente
            signal(SIGCHLD,SIG_DFL);
            signal(SIGINT,SIG_IGN);
            //ejecutamos comando externo 
            execvp(args[0],args);
            //imprimimos mensaje de error en 
            //caso de que este se produzca   
            perror("Error");
            exit(-1);
        }else if(jobs_list[0].pid>0){
            signal(SIGINT,ctrlc);
            fprintf(stderr,GRIS_T "[execute_line()->PID padre: %d (%s)]\n" RESET, getpid(),mi_shell);
            fprintf(stderr,GRIS_T "[execute_line()->PID hijo: %d (%s)]\n" RESET, jobs_list[0].pid,line2);
            //actualizamos el estado en ejecución
            jobs_list[0].status='E';
            //mientras haya un proceso hijo ejecutándose
            //en primer plano estará esperando hasta que 
            //llegue alguna señal
            while(jobs_list[0].pid>0){
                pause();
            }
            strcpy(jobs_list[0].cmd,line2);
            //fprintf(stderr,GRIS_T "[execute_line()->proceso hijo: %d (%s)finalizado con exit(),estado: 0]\n" RESET, jobs_list[0].pid, line2);
        //en caso de que esté en segundo plano
        }else{
            exit(-1);
        }
    }
}

//troceamos la línea pasada por teclado en tokens y lo almacenamos en args
int parse_args(char **args, char *line) {
    char *token;
    char s[4] = " \t\n\r";
    int num = 0;
    token = strtok(line,s);
    //printf(GRIS_T "[parse_args()->token %d: %s]\n" RESET, num, token);  
    args[num] = token;
    num++;
    while(token != NULL) {
        if(token[0] == '#') {
            --num;
            args[num]=NULL;
            token = NULL;
            //printf(GRIS_T "[parse_args()->token %d corregido: %s]\n" RESET, num, token);
        }
        else {
            token = strtok(NULL,s);
            //printf(GRIS_T "[parse_args()->token %d: %s]\n" RESET, num, token);
            args[num] = token;
            num++;
        }
    }
    return num;
}

//comprobamos si se trata de un comando interno
//y, si es así, miramos de qué tipo se trata
int check_internal(char **args){
    if(args[0]==NULL){
        return 0;
    }
    int cmp= strcmp(args[0],"cd");
    if(cmp==0){
        internal_cd(args);
        return 1;
    }
    cmp=strcmp(args[0],"export");
    if(cmp==0){
        internal_export(args);
        return 1;
    }
    cmp=strcmp(args[0],"source");
    if(cmp==0){
        internal_source(args);
        return 1;
    }
    cmp=strcmp(args[0],"jobs");
    if(cmp==0){
        internal_jobs(args);
        return 1;
    }
    cmp=strcmp(args[0],"fg");
    if(cmp==0){
        internal_fg(args);
        return 1;
    }
    cmp=strcmp(args[0],"bg");
    if(cmp==0){
        internal_bg(args);
        return 1;
    }
    cmp=strcmp(args[0],"exit");
    if(cmp==0){
        exit(0);
    }else{
        return 0;
    }

}

//función para imprimir el directorio en el que
//nos encontramos
int internal_cd(char **args){
    if(chdir(args[1])==-1&&args[1]!=NULL){
        printf("chdir: No such file or directory \n");
    }else{
        char cwd[COMMAND_LINE_SIZE];
        if (getcwd(cwd, COMMAND_LINE_SIZE) != NULL) {
            fprintf(stderr, GRIS_T "[internal_cd()→ PWD: %s]\n" RESET,cwd);
        }
    }
}

//función con la finalidad de cambiar los valores
//de las variables de entorno del mini shell
int internal_export(char **args){
    const char ch[2]="=";
    char *nombre;
    char *valor;
    if(args[1]!=NULL){
        nombre=strtok(args[1],ch);
        valor=strtok(NULL,ch);
        if(valor!=NULL){
            fprintf(stderr, GRIS_T "[internal_export()→ nombre: %s]\n" RESET,nombre);
            fprintf(stderr, GRIS_T "[internal_export()→ valor: %s]\n" RESET,valor);
            fprintf(stderr, GRIS_T "[internal_export()→ antiguo valor para USER: %s]\n" RESET,getenv("USER"));
            setenv("USER",nombre,1);
            fprintf(stderr, GRIS_T "[internal_export()→ nuevo valor para USER: %s]\n" RESET,getenv("USER"));
        
        }else{
            fprintf(stderr, ROJO_T "Error de sintaxis. Uso: export Nombre = Valor\n" RESET);
        }

    }else{
        fprintf(stderr, ROJO_T "Error de sintaxis. Uso: export Nombre = Valor\n" RESET);
    }
}

//función para ejecutar los comandos en 
//el fichero indicado
int internal_source(char **args){
    if(args[1]==NULL){
        fprintf(stderr, ROJO_T "Error de sintaxis. Uso: source<Nombre.fichero>\n" RESET);
    }else{
        FILE *fp=fopen(args[1],"r");
        if(fp==NULL){
            fprintf(stderr, ROJO_T "fopen:No such file or directory" RESET);
        }else{
            char texto[100];
            while(fgets(texto,99,fp)!=NULL){
                texto[strlen(texto)-1]='\0';
                execute_line(texto);
                fflush(fp);
            }
            fclose(fp);
        }
    }
    
    fprintf(stderr, GRIS_T "[internal_source()→ es un source]\n" RESET);

}

int internal_jobs(char **args){
    fprintf(stderr, GRIS_T "[internal_jobs()→ es un jobs]\n" RESET);
}

int internal_fg(char **args){
    fprintf(stderr, GRIS_T "[internal_fg()→ es un fg]\n" RESET);
}

int internal_bg(char **args){
    fprintf(stderr, GRIS_T "[internal_bg()→ es un bg]\n" RESET);

}