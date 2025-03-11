//Cristóbal Albertí Martínez
//Pedro García Lozano
//Yi Xu

#include <stdio.h>  
#include <string.h> 
#include <stdlib.h>  
#include <fcntl.h>    
#include <sys/wait.h>
#include <unistd.h>   
#include <errno.h>     
#include <sys/types.h>  
#include <sys/stat.h>  
#include <signal.h>
#include <stdbool.h>

#define _POSIX_C_SOURCE_200112L
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define PROMPT '$'
#define AZUL_T "\x1b[34m"
#define RESET "\033[0m"
#define GRIS_T "\x1b[90m"
#define ROJO_T "\x1b[31m"
#define NEGRO_T "\x1b[30m"
#define MAGENTA_T "\x1b[35m"
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
int is_output_redirection(char **args);

struct info_job {
   pid_t pid;
   char status; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[COMMAND_LINE_SIZE]; // línea de comando asociada
};
static struct info_job jobs_list[N_JOBS];

static char mi_shell[COMMAND_LINE_SIZE];

static int n_pids = 0;

//esta función imprime el prompt y lee la línea pasada por 
//teclado y lo almacena en una variable line que será lo 
// que devuelva la función

char *read_line(char *line){
    imprimirPROMPT();
    fgets(line,1023,stdin);
    if(feof(stdin)){
        exit(0);
    }
    line[strlen(line)-1]='\0';
    return (char*) line;
}

void imprimirPROMPT(){
    char cwd[COMMAND_LINE_SIZE];
    getcwd(cwd, COMMAND_LINE_SIZE);
    printf(MAGENTA_T "%s" RESET ":" AZUL_T "%s " NEGRO_T "%c " , getenv("USER"), cwd, PROMPT);
    fflush(stdout);
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
            jobs_list[0].pid=0;
            jobs_list[0].status='F';
            for(int i=0;i<N_JOBS;i++){
            jobs_list[0].cmd[i]='\0';
        }
    }
        else {
            ended = jobs_list_find(ended);
            printf("Terminado PID %d (%s) en jobs_list[%d] con status %d\n", jobs_list[ended].pid, jobs_list[ended].cmd, ended, WEXITSTATUS(jobs_list[ended].status));
            jobs_list_remove(ended);
        }
    }
 
}

//Finaliza el proceso que se ecuentra en primer plano.
//Este proceso maneja la señal SIGINT, la cual se activa 
//en caso de usar ctrl+c.

void ctrlc(int signum){
    char mensaje[1200];
    signal(SIGINT,ctrlc);
    if(jobs_list[0].pid>0){
        if(strcmp(jobs_list[0].cmd,mi_shell)!=0){
            kill(jobs_list[0].pid,SIGTERM);
        }
    }
    printf("\n");
    fflush(stdout);
}

//Manejador de la señal SIGTSTP, cuya activación viene
//dada por el uso de ctrl+z. Detiene el proceso que se
//encuentra en primer plano y lo nvía con estado detenido
//al segundo plano.

void ctrlz(int signum){
    char mensaje[1200];
    signal(SIGTSTP,ctrlz);
    printf("\n");
    if(jobs_list[0].pid>0){
        if(jobs_list[0].cmd!=mi_shell){
            kill(jobs_list[0].pid,SIGSTOP);
            jobs_list[0].status = 'D';
            jobs_list_add(jobs_list[0].pid, jobs_list[0].status, jobs_list[0].cmd);
            jobs_list[0].pid=0;
            jobs_list[0].status='F';
            for(int i=0;i<N_JOBS;i++){
                jobs_list[0].cmd[i]='\0';
            }
        }
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
    int is_bg;
    //condicional que comprueba si se 
    //trata de un proceso interno
    if(check_internal(args)==0){
        //creamos un proceso hijo 
        //del mini shell 
        is_bg = is_background(args);
        pid_t pid = fork();
        //a través de este condicional
        //comprobamos si se trata de un proceso 
        //hijo
        if(pid==0){
            //declaramos las distintas señales
            //para el reaper, el ctrlc y el ctrlz
            //respectivamente
            signal(SIGCHLD,SIG_DFL);
            signal(SIGINT,SIG_IGN);
            signal(SIGTSTP,SIG_IGN);
            //miramos si se trata de un redireccionamiento
            is_output_redirection(args);
            //ejecutamos comando externo    
            execvp(args[0],args);
            //imprimimos mensaje de error en 
            //caso de que este se produzca
            fprintf(stderr, ROJO_T "%s: no se encontró la orden\n" RESET, line2);
            exit(-1);
            //comprobamos si es un proceso padre 
        }else if(pid>0){
            //miramos si está en primer plano 
            if(is_bg == 0) {
                jobs_list[0].pid=pid;
                jobs_list[0].status='E';
                //copiamos en jobs_list cmd lo que 
                //ha sido introducido por teclado
                strcpy(jobs_list[0].cmd,line2);
                //mientras haya un proceso hijo ejecutándose
                //en primer plano estará esperando hasta que 
                //llegue alguna señal
                while(jobs_list[0].pid>0){
                    pause();
                }
            //en caso de que esté en segundo plano
            }else{
                //añadimos un nuevo proceso al segundo plano
                jobs_list_add(pid, 'E', line2);
                //reseteamos los valores de jobs_list[0]
                jobs_list[0].pid=0;
                jobs_list[0].status='F';
                for(int i=0;i<N_JOBS;i++){
                    jobs_list[0].cmd[i]='\0';
                }

            }
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
    args[num] = token;
    num++;
    while(token != NULL) {
        if(token[0] == '#') {
            --num;
            args[num]=NULL;
            token = NULL;
        }
        else {
            token = strtok(NULL,s);
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
    char cwd[COMMAND_LINE_SIZE];
    if(args[1]==NULL){
        chdir(getenv("HOME"));
    }else{
        getcwd(cwd, COMMAND_LINE_SIZE);
        while (args[1][0] == args[1][1] && args[1][0] == '.') {
            while (cwd[strlen(cwd) - 1] != '/'){
                cwd[strlen(cwd) - 1] = '\0';
            } 
            cwd[strlen(cwd) - 1] = '\0';          
            args[1]++;                            
            args[1]++;
            if (args[1][0] == '/'){
                args[1]++; 
            }
        }
        char argscwd[COMMAND_LINE_SIZE * ARGS_SIZE];
        memset(argscwd, 0, COMMAND_LINE_SIZE * ARGS_SIZE);           
        int i = 1;                                           
        while (args[i] != NULL && args[i][0] != '\0'){
            strcat(argscwd, "/");
            while (args[i] != NULL && args[i][0] != '\0' && args[i][strlen(args[i]) - 1] == 92) {
                args[i][strlen(args[i]) - 1] = '\0';
                strcat(argscwd, args[i]);
                strcat(argscwd, " ");
                i++;
            }
            if (args[i][0] == 34 || args[i][0] == 39){
                args[i]++; 
                while (args[i] != NULL && args[i][0] != '\0' && !(args[i][strlen(args[i]) - 1] == 34 || args[i][strlen(args[i]) - 1] == 39)){
                    strcat(argscwd, args[i]);
                    strcat(argscwd, " ");
                    i++;
                }
                args[i][strlen(args[i]) - 1] = '\0';
            }
            strcat(argscwd, args[i]);
            i++; 
        }
        strcat(cwd, argscwd);
        if (chdir(cwd)){
            perror("chdir:");
            return -1;
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
            setenv(nombre,valor,1);
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
    }else{
        FILE *fp=fopen(args[1],"r");
        if(fp==NULL){
        }else{
            char texto[100];
            while(fgets(texto,99,fp)!=NULL){
                texto[strlen(texto)-1]='\0';
                fflush(fp);
                execute_line(texto);   
            }
            fclose(fp);
        }
    }
}

//fucnión para impripor pantalla 
//los procesos que se encuentran en segundo plano

int internal_jobs(char **args){
    for(int i = 1; i <= n_pids; i++) {
        printf("[%d]%d   %c    %s\n", i, jobs_list[i].pid, jobs_list[i].status, jobs_list[i].cmd);
    }
}

int internal_fg(char **args){
    //asignamos a la posición el valor que nos pasan por
    //teclado y lo convertimos a int
    int pos=atoi(args[1]);
    //comprobamos que el proceso no se encuentre en primer
    //plano. En caso de ser así, proporcionará un error
    if(pos > n_pids || pos == 0){
        printf(ROJO_T "%s %s: no existe ese trabajo\n" RESET, args[0], args[1]);
    }else{
        //miramos si el proceso se encuentra en segundo plano y detenido
        if(jobs_list[pos].status == 'D'){
            //enviamos una señal SIGCONT
            kill(jobs_list[pos].pid,SIGCONT);
        }
        //enviamos los valores del proceso en la posición pos 
        //al proceso que se encuentra en la posición 0 (primer plano)
        jobs_list[0].pid=jobs_list[pos].pid;
        jobs_list[0].status = 'E';
        for(int i =0;(i<N_JOBS) && (jobs_list[pos].cmd[i]!='&');i++){
            jobs_list[0].cmd[i]=jobs_list[pos].cmd[i];
        }
        //elimina el proceso en la posición pos
        jobs_list_remove(pos);
        printf("%s\n",jobs_list[0].cmd);
        //mientras haya un proceso en primer plano,
        //se ejecutará pause()
        while(jobs_list[0].pid>0){
            pause;
        }
    }
}

int internal_bg(char **args){
    //asignamos a la posición el valor que nos pasan por
    //teclado y lo convertimos a int
    int pos = atoi(args[1]);
    //si el proceso se encuentra en primer plano o es mayor 
    //que el número de procesos existentes, quiere decir que no existe
    if(pos > n_pids || pos == 0){
        printf(ROJO_T "%s %s: no existe ese trabajo\n" RESET, args[0], args[1]);
        //si el estado del proceso en segundo plano es "en ejecución", 
        //proporcionará un error
    }else if(jobs_list[pos].status == 'E'){
        printf(ROJO_T "%s %s: el trabajo ya está en segundo plano\n" RESET, args[0], args[1]);
        //en caso que el proceso en segundo plano se encuentre detenido
    }else{
        //el estado pasa a estar en ejecución
        jobs_list[pos].status = 'E';
        //le añadimos un & al nombre del proceso
        strcat(jobs_list[pos].cmd," &");
        //envía una señal SIGCONT a la posición jobs_list[pos].pid
        kill(jobs_list[pos].pid,SIGCONT);
        printf("[%d]%d   %c    %s\n", pos, jobs_list[pos].pid, jobs_list[pos].status, jobs_list[pos].cmd);
    }

}

//comprobamos si la línea introducida por teclado tiene 
//un carácter & y de esta manera comprobamos si se encuentra 
//en segundo plano

int is_background(char **args) {
    if(args[0]==NULL){
        return 0;
    }
    int i=0;
    while(args[i]!=NULL){
        i++;
    }
    if(strcmp(args[i-1],"&")==0){
        args[i-1]=NULL;
        return 1;
    }
    return 0;
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
    if(n_pids<1){
        return -1;
    }
    
    jobs_list[pos].pid = jobs_list[n_pids].pid;
    jobs_list[pos].status = jobs_list[n_pids].status;
    for(int j = 0; j <= N_JOBS; j++) {
        jobs_list[pos].cmd[j] = jobs_list[n_pids].cmd[j];
    }
    jobs_list[n_pids].pid=0;
    jobs_list[n_pids].status = 'N';
    memset(jobs_list[n_pids].cmd, '\0', COMMAND_LINE_SIZE);
    n_pids = n_pids - 1;
}

//función que, en caso de que se encuentre el símbolo 
//">", abrimos el fichero al cual queremos redireccionar 
//la salida del comando a ejecutar el fichero

int is_output_redirection(char **args){
    int i=0;
    while(args[i]!=NULL){
        if((*args[i]=='>')&& (args[i+1]!=NULL)){
            int fd=open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR,S_IWUSR);
            if(fd<0){
                printf("Error");
                exit(0);
            }else{
                args[i]=NULL;
                dup2(fd,1);
                close(fd);
            }

        }else{
            i++;
        }   
    }
}

int main (int argc, char *argv[]) {
    //guardamos el comando de ejecución
    //en una variable de tipo char
    strcpy(mi_shell,argv[0]);
    //inicializamos los valores de jobs_list[0] (PID, estado y nombre)
    jobs_list[0].pid=0;
    jobs_list[0].status='N';
    
    for(int i=0;i<N_JOBS;i++){
        jobs_list[0].cmd[i]='\0';
    }
    //asignamos las señales del reaper, el ctrlc y ctrlz
    signal(SIGCHLD,reaper);
    signal(SIGINT,ctrlc);
    signal(SIGTSTP,ctrlz);

    char line[COMMAND_LINE_SIZE];
    //bucle infinito para leer las 
    //líneas y ejecutarlas
    while(1){
        if(read_line(line)){
            execute_line(line);
            memset(line, '\0', COMMAND_LINE_SIZE);
        }
    }
    return 0;
}
