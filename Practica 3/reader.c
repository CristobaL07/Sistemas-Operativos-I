#include <limits.h>
#include "my_lib.h"
// Cristóbal Arístides Albertí Martínez
// Pedro García Lozano
// Yi Xu

struct my_data {
    int val;
    char name[100];
};
struct my_data *data1;

int main (int argc, char *argv[]) {
    // comprobamos que exista el archivo que quisiera leer
    if(my_stack_read(argv[1])) {
        // leemos el archivo y lo guardamos en la pila
        struct my_stack *f1 = my_stack_read(argv[1]);
        printf("Stack length: %d\n", my_stack_len(f1));
        // sacamos los elementos de la pila los guardamos en una variable local
        // y los vamos imprimiendo
        data1=my_stack_pop(f1);
        long num[my_stack_len(f1)];
        for (int i = 0; i<10; i++) {
            num[i] = data1->val;
            printf("%ld\n", num[i]);
            data1 = my_stack_pop(f1);
        }
        // declaramos las variables para la suma, mínimo, máximo y media
        long sum = 0; 
        long min, max, average;
        // sumamos todos los elementos de la pila
        for (int i = 0; i < 10; i++) {
            sum = sum + num[i];
        }
        // buscamos el elemento más pequeño de la pila
        min = num[0];
        for (int i = 0; i < 10; i++) {
            if(min > num[i]) {
                min = num[i];
            }
        }
        // buscamos el elemento más grande de la pila
        max = num[0];
        for (int i = 0; i < 10; i++) {
            if(max < num[i]) {
                max = num[i];
            }
        }
        // dividimos la suma de todos los elementos de la pila entre número de elementos
        average = sum / 10;
        printf("Items: 10  Sum: %ld  Min: %ld  Max: %ld  Average: %ld\n", sum, min, max, average);
    }
    else if(argv[1] == NULL) {
        printf("USAGE:./reader atack_file\n");
        exit(0);
    }
    else {
        printf("Couldn't open stack file %s\n", argv[1]);
    }
}