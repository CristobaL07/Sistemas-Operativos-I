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
void ctrlz(int signum);
int is_background(char **args);
int jobs_list_add(pid_t pid, char status, char *cmd);
int jobs_list_find(pid_t pid);
int jobs_list_remove(int pos);

struct info_job {
   pid_t pid;
   char status; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[COMMAND_LINE_SIZE]; // línea de comando asociada
};
static struct info_job jobs_list[N_JOBS];

static char mi_shell[COMMAND_LINE_SIZE];

static int n_pids = 0;

int main (int argc, char *argv[]) {
    //guardamos el comando de ejecución
    //en una variable de tipo char
    if(argc == 1){
        int cmp = strcmp(argv[0],"./nivel5");
        if(cmp == 0){
            strcpy(mi_shell,argv[0]);
        }
    }
    //inicializamos los valores de jobs_list[0] (PID, estado y nombre)
    jobs_list[0].pid=0;
    jobs_list[0].status='N';
    for(int i=0;i<N_JOBS;i++){
        jobs_list[0].cmd[i]='\0';
    }
    //asignamos las señales del reaper, el ctrlc y ctrlz
    signal(SIGCHLD,reaper);
    signal(SIGINT,ctrlc);
    signal(SIGSTOP,ctrlz);

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

char *read_line(char *line) {
    printf("%c ",PROMPT);
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

void reaper(int signum) {
    char mensaje[1200];
    signal(SIGCHLD,reaper);
    int value;
    int ended = waitpid(-1, &value, WNOHANG);
    if(ended != 0) {
        if(ended == jobs_list[0].pid) {
        ended = jobs_list_find(ended);
        printf(GRIS_T "[reaper()-> recibida señal 17 (SIGCHLD)]\n" RESET);
        printf(GRIS_T "[reaper()-> Proceso hijo %d en foreground (%s) finalizado por señal 15]\n" RESET, jobs_list[ended].pid, jobs_list[ended].cmd);
        jobs_list[0].pid=0;
        jobs_list[0].status='F';
            for(int i=0;i<N_JOBS;i++){
                jobs_list[0].cmd[i]='\0';
            }
        }
    else {
        ended = jobs_list_find(ended);
        printf(GRIS_T "\n[reaper()-> recibida señal 17 (SIGCHLD)]\n" RESET);
        printf(GRIS_T "[reaper()-> Proceso hijo %d en background (%s) finalizado con exit code %d]\n" RESET, jobs_list[ended].pid, jobs_list[ended].cmd, WEXITSTATUS(jobs_list[ended].status));
        printf("Terminado PID %d (%s) en jobs_list[%d] con status %d\n", jobs_list[ended].pid, jobs_list[ended].cmd, ended, WEXITSTATUS(jobs_list[ended].status));
        jobs_list_remove(ended);
        }
    }
}

//Manejador de la señal SIGTSTP, cuya activación viene
//dada por el uso de ctrl+z. Detiene el proceso que se
//encuentra en primer plano y lo nvía con estado detenido
//al segundo plano.

void ctrlz(int signum){
    signal(SIGTSTP,ctrlz);
    printf("\n[ctrlz()-> Soy el proceso con PID %d, el proceso en foreground es %d (%s)]\n",getpid(),jobs_list[0].pid, jobs_list[0].cmd);
    if(jobs_list[0].pid>0){
        if(jobs_list[0].cmd!=mi_shell){
            printf("[ctrlz()-> recibida señal 20 (SIGTSTP)]\n");

            kill(jobs_list[0].pid,SIGSTOP);
            jobs_list[0].status = 'D';
            printf("[ctrlz()-> señal 19 (SIGSTOP enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid, jobs_list[0].cmd,getpid(),mi_shell);
            jobs_list_add(jobs_list[0].pid, jobs_list[0].status, jobs_list[0].cmd);
            jobs_list[0].pid = 0;
            jobs_list[0].status = 'F';
            for(int i=0;i<N_JOBS;i++){
                jobs_list[0].cmd[i]='\0';
            }
        }else{
            printf("[ctrlz()-> señal SIGSTOP no enviado por %d (%s) debido a que su proceso en foreground es el shell]\n",jobs_list[0].pid, 
            jobs_list[0].cmd);
        }
    }else{
        printf("[ctrlz()-> señal SIGSTOP no enviado por %d (%s) debido a que no hay proceso en foreground]\n",jobs_list[0].pid, jobs_list[0].cmd);
        printf("%c ",PROMPT);
    }
    fflush(stdout);
}

//Finaliza el proceso que se ecuentra en primer plano.
//Este proceso maneja la señal SIGINT, la cual se activa 
//en caso de usar ctrl+c.

void ctrlc(int signum){
    signal(SIGINT,ctrlc);
    printf("\n[ctrlc()-> Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),mi_shell,jobs_list[0].pid, jobs_list[0].cmd);
    printf("[ctrlc()-> recibida señal 2 (SIGINT)]\n");
    if(jobs_list[0].pid>0){
        if(jobs_list[0].cmd!=mi_shell){
            printf("[ctrlc()-> señal SIGTERM enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid, jobs_list[0].cmd, getpid(),mi_shell);
            kill(jobs_list[0].pid,SIGTERM);
        }else{
            printf("[ctrlc()-> señal SIGTERM no enviado por %d (%s) debido a que su proceso en foreground es el shell]\n",
            jobs_list[0].pid, jobs_list[0].cmd);
        }
    }else{
        printf("[ctrlc()-> señal 15 (SIGTERM) no enviado por %d (%s) debido a que no hay proceso en foreground]\n",getpid(), mi_shell);
        printf("%c ",PROMPT);
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
    //miramos si lo que se ha introducido
    //es un proceso en segundo plano
    bool is_bg = is_background(args);
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
            //para el reaper, el ctrlc y el ctrlz
            //respectivamente
            signal(SIGCHLD,SIG_DFL);
            signal(SIGINT,SIG_IGN);
            signal(SIGTSTP,SIG_IGN);
            //ejecutamos comando externo    
            execvp(args[0],args);
            //imprimimos mensaje de error en 
            //caso de que este se produzca
            perror("Error");
            exit(-1);
        //comprobamos si es un proceso padre
        }else if(jobs_list[0].pid>0){
            //copiamos en jobs_list cmd lo que 
            //ha sido introducido por teclado
            for(int i = 0; i < N_JOBS; i++) {
                jobs_list[0].cmd[i] = line2[i];
            }
            fprintf(stderr,GRIS_T "[execute_line()->PID padre: %d (%s)]\n" RESET, getpid(),mi_shell);
            fprintf(stderr,GRIS_T "[execute_line()->PID hijo: %d (%s)]\n" RESET, jobs_list[0].pid,jobs_list[0].cmd);
            //actualizamos el estado en ejecución
            jobs_list[0].status='E';
            //miramos si está en primer plano 
            if(is_bg == false) {
                signal(SIGINT,ctrlc);
                signal(SIGTSTP,ctrlz);
                //mientras haya un proceso hijo ejecutándose
                //en primer plano estará esperando hasta que 
                //llegue alguna señal
                while(jobs_list[0].pid>0){
                    pause();
                }
            }
            //fprintf(stderr,GRIS_T "[execute_line()->proceso hijo: %d (%s)finalizado con exit(),estado: 0]\n" RESET, jobs_list[0].pid, line2);
            //en caso de que esté en segundo plano
            else {
                //añadimos un nuevo proceso al segundo plano
                jobs_list_add(jobs_list[0].pid, jobs_list[0].status, jobs_list[0].cmd);
                //reseteamos los valores de jobs_list[0]
                jobs_list[0].pid=0;
                jobs_list[0].status='N';
                for(int i=0;i<N_JOBS;i++){
                    jobs_list[0].cmd[i]='\0';
                }
            }        
        }
        else{
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

//fucnión para impripor pantalla 
//los procesos que se encuentran en segundo plano
int internal_jobs(char **args){
    for(int i = 1; i <= n_pids; i++) {
        printf("[%d]%d   %c    %s\n", i, jobs_list[i].pid, jobs_list[i].status, jobs_list[i].cmd);
    }
}

int internal_fg(char **args){
    fprintf(stderr, GRIS_T "[internal_fg()→ es un fg]\n" RESET);
}

int internal_bg(char **args){
    fprintf(stderr, GRIS_T "[internal_bg()→ es un bg]\n" RESET);

}

//comprobamos si la línea introducida por teclado tiene 
//un carácter & y de esta manera comprobamos si se encuentra 
//en segundo plano
int is_background(char **args) {
    char backg[1];
    if(args[2] != NULL) {
        strncpy(backg, args[2], 1);
        if ('&' == backg[0]) {
            args[2] = NULL;
            return 1;
        }
        else {
            return 0;
        }
    }
}

//función para añadir un nuevo proceso a la lista de procesos en segundo plano
//y aumentamos en uno el número de procesos totales
int jobs_list_add(pid_t pid, char status, char *cmd) {
    n_pids = n_pids + 1;
    jobs_list[n_pids].pid = pid;
    jobs_list[n_pids].status = status;
    for(int i = 0; i <= N_JOBS; i++) {
        jobs_list[n_pids].cmd[i] = cmd[i];
    }
    printf("[%d]%d   %c    %s\n", n_pids, jobs_list[n_pids].pid, jobs_list[n_pids].status, jobs_list[n_pids].cmd);
}

//mediante el pid pasado por parámetro encontramos
//el proceso al cual pertenece
int jobs_list_find(pid_t pid) {
    for(int i = 0; i <= n_pids; i++) {
        if(jobs_list[i].pid == pid) {
            return i;
        }
    }
}

//función para eliminar el proceso en la posición
//pasada por parámetro. Movemos los procesos 
//posteriores a una posición anterior y reducimos
//el número de procesos totales en uno
int jobs_list_remove(int pos) {
    for (int i = pos; i < n_pids; i++) {
        jobs_list[i].pid = jobs_list[i+1].pid;
        jobs_list[i].status = jobs_list[i+1].status;
        for(int j = 0; j <= N_JOBS; j++) {
            jobs_list[i].cmd[j] = jobs_list[i+1].cmd[j];
        }
    }
    n_pids = n_pids - 1;
}