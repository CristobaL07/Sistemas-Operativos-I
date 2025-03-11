#include <pthread.h>
#include "my_lib.h"
// Cristóbal Arístides Albertí Martínez
// Pedro García Lozano
// Yi Xu

#define NUM_THREADS 10

pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
struct my_stack *f1;
struct my_data *data,*data2;
int N = 1000000;

struct my_data {
    int val;
    char name[100];
};

void *worker(void *ptr);


int main (int argc, char *argv[]){
    // comprobamos que exista el archivo
    if(argv[1]!=NULL){
        // leemos el archivo y lo guardamos en la pila
        f1=my_stack_read(argv[1]);
        // comprobamos que el archivo no esté vacío
        if(f1==NULL){
            printf("Threads: %d, Iterations: %d\n", NUM_THREADS, N);
            // creamos una pila nueva
            f1 = my_stack_init(sizeof(struct my_data));
            printf("Initial stack length: %d\n", my_stack_len(f1));
            printf("Initial stack content:\n");
            printf("Stack content for treatment:\n");
            // si hay menos de 10 elementos introducimos los elementos restantes
            while(my_stack_len(f1)<NUM_THREADS){
                data = malloc(sizeof(struct my_data));
                data->val = 0;
                my_stack_push(f1,data);
                printf("%d\n",data->val);
            }
            printf("New stack length: %d\n", my_stack_len(f1));
            printf("\n");
        }
        // si hay algo en el archivo
        else{
            printf("Threads: %d, Iterations: %d\n", NUM_THREADS, N);
            printf("Initial stack length: %d\n", my_stack_len(f1));
            printf("Original stack content:\n");
            // creamos una pila nueva para imprimir todos sus elementos
            struct my_stack *f2=my_stack_read(argv[1]);
            int num[my_stack_len(f1)];
            for(int i=0;i<my_stack_len(f1);i++){
                data2=my_stack_pop(f2);
                printf("%d\n",data2->val);
                num[i]=data2->val;
            }
            printf("Original stack content length:%d\n",my_stack_len(f1));
            // si en el archivo hay menos de 10 elementos
            if(my_stack_len(f1)<NUM_THREADS){
                printf("Number of elements added to the initial stack %d\n", NUM_THREADS - my_stack_len(f1));
                printf("Stack content for treatment:\n");
                // imprimimos todos los elementos que se van a iterar
                for(int i=0;i<my_stack_len(f1);i++){
                    printf("%d\n",num[i]);
                }
            }
            // si en el archivo hay menos de 10 elementos los añadimos 
            while(my_stack_len(f1)<NUM_THREADS){
                data = malloc(sizeof(struct my_data));
                data->val = 0;
                printf("%d\n",data->val);
                my_stack_push(f1,data);
            }
            printf("Stack length: %d\n", my_stack_len(f1));
            printf("\n");
        }
    }else{
        printf("USAGE:./stack_counters atack_file\n");
        exit(0);
    }
    // creamos los 10 hilos y los enviamos al worker
    pthread_t thread[NUM_THREADS];
    int r;
    for(int t=0;t<NUM_THREADS;t++){
        r=pthread_create(&thread[t],NULL,worker,NULL);
        if(r){
            printf("Error");
            exit(-1);
        }
    }
    // duerme los hilos hasta otros hilos se terminen
    for(int t=0;t<NUM_THREADS;t++){
        pthread_join(thread[t],NULL);
    }
    // escribimos las pilas modificadas en el archivo
    my_stack_write(f1,argv[1]);
    printf("\n");
    printf("Stack content after threads iterations\n");
    int size=my_stack_len(f1);
    data2=my_stack_pop(f1);
    int num1[my_stack_len(f1)];
    // imprimimos los elementos de la pila ya modificados
    for (int i = 0; i<size; i++) {
        num1[i] = data2->val;
        printf("%d\n", num1[i]);
        data2 = my_stack_pop(f1);
    }
    printf("Stack length: %d\n",size);
    printf("\n");
    printf("Written elements from stack to file: %d\n",size);
    f1=my_stack_read(argv[1]);
    // liberamos la pila
    printf("Released bytes: %d\n",my_stack_purge(f1));
    printf("Bye from main");
    pthread_exit(NULL);
    
}
void *worker(void *ptr){
    printf ( "Thread  %ld created \n" , pthread_self()) ;
    struct my_data *data1;
    // hacemos un millón de iteraciones entre los diferentes hilos
    for(int n=0;n<N;n++){
        pthread_mutex_lock(&m);
        data1=my_stack_pop(f1);
        pthread_mutex_unlock(&m);
        pthread_mutex_lock(&m);
        data1->val++;
        pthread_mutex_unlock(&m);
        pthread_mutex_lock(&m);
        my_stack_push(f1,data1);
        pthread_mutex_unlock(&m);
    }
    pthread_exit(NULL);
    
}