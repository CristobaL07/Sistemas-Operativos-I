/* my_lib.c */
#include "my_lib.h"

// Cristóbal Arístides Albertí Martínez
// Pedro García Lozano
// Yi Xu
 
/************************************************************************/

size_t my_strlen(const char *str) {
    size_t len = 0;
    int i = 0;
    while (str[i]!='\0') {
           i++;
           len++;
    }
   return len;
}

/************************************************************************/

int my_strcmp (const char *str1, const char *str2) {
       int cmp = 0;
       int mayor;
       // miramos cual de los dos es el mas corto para asi poder compararlos
       mayor = strlen(str1) - strlen(str2);
       // aqui comparamos los elementos respecto a la longitud de str1 ya que es el menor
       if (mayor < 0) {
              mayor = 0;
              // hacemos un bucle restando los elementos de cada uno, para que de esta manera saber el resultado dependiendo de si es postivo o negativo
              for (int i = 0; i < strlen(str1); i++) {
                     mayor = mayor + (str1[i] - str2[i]);
              }
              // si es negativo sabemos que str2 es mayor
              if (mayor < 0) {
                     cmp--;
              }
              // si es positivo sabemos que str1 es mayor
              else if (mayor > 0) {
                     cmp++;
              }
       }
       // aqui comparamos los elementos respecto a la longitud de str2 ya que es el menor
       else {
              mayor = 0;
              // realizamos el mismo bucle del caso anterior solo que utilizando la longitud de str2
              for (int i = 0; i< strlen(str2); i++) {
                     mayor = mayor + (str1[i] - str2[i]);
              }
              if (mayor < 0) {
                     cmp--;
              }
              else if (mayor > 0) {
                     cmp++;
              } 
       }
       // devolvemos cmp que tendra de valor -1,0,1 dependiendo del resultado
       return cmp;
}

/************************************************************************/

char *my_strcpy(char *dest, const char *src) {
       int length;
       // creamos un bucle de donde copiamos los elementos, de uno en uno hasta que haya un espacio vacio, del string que nos pasan al string destino
       for (int i = 0; src[i] != '\0'; i++) {
              dest[i] = src[i];
              // guardamos la longitud de todos los elementos copiados en una variable
              length = i;
       }
       // usamos la variable con la longitud para agregar un espacio vacio al final
       dest[length + 1] = '\0';
       // devolvemos el string destino
       return dest;
}

/************************************************************************/

char *my_strncpy(char *dest, const char *src, size_t n) {
       // creamos un bucle que copia una cantidad que nos dan de elementos del string que nos dan al string destino
       for (int i = 0; i < n;i ++) {
              dest[i] = src[i];
       }
       // devolvemos el string destino con la cantidad de elementos que nos han dicho que copiemos
       return dest;
}

/************************************************************************/

char *my_strcat(char *dest, const char *src) {
       int y = 0;
       // hacemos un bucle en el que nos desplaza hasta la posicion en la cual aparece el primer espacio en blanco
       while (dest[y] != '\0') {
              y++;
       }
       // usamos la variable donde tenemos la posicion con el primer espacio en blanco para hacer un bucle justo en el espacio en blanco
       // el bucle copia los elementos del string que nos dan en el destino desde la posicion de la variable y, lo que hace que ponga los elementos justo detrasde los elementos ya existentes
       for (int x = 0; src[x] != '\0'; x++) {
              dest[y] = src[x];
              y++;
       }
       // devolvemos el string destino el cual ahora tiene sus elementos directamente seguidos de los que han sido copiados
       return dest;
}

/************************************************************************/

char *my_strchr(const char *s, int c) {
       int y = 0;
       // hacemos un bucle en el que nos desplaza hasta la posicion en la cual aparece el primer elemento que nos han proporcionado
       while (s[y] != c) {
              y++;
       }
       // devolvemos el string que nos han dado apuntando a su posicion donde esta el primer elemento que nos han dado
       return (char*)s + y;
}

/************************************************************************/

struct my_stack *my_stack_init(int size) {
       // inicializamos la pila con el tamaño que nos proporcionan
       struct my_stack *stack = (struct my_stack*)malloc(sizeof(struct my_stack));
       // inicializamos top con NULL ya que empieza vacia la pila
       stack->size=size;
       stack->top = NULL;
       // devolvemos la pila inicializada
       return stack;
}

/************************************************************************/


int my_stack_push(struct my_stack *stack, void *data){
       // creamos un nuevo nodo
       struct my_stack_node *nuevo_nodo=(struct my_stack_node *)malloc(sizeof(struct my_stack_node));
       // el siguiente elemento adquiere el valor del top, mientras el top pasa a ser el elemento que nos dan
       nuevo_nodo->data=data;
       nuevo_nodo->next=stack->top;
       stack->top=nuevo_nodo;
       // devolvemos 0 si ha ido bien
       return 0;

}

void *my_stack_pop(struct my_stack *stack){
       // creamos un puntero para poder extraer el elemnto de la cima de la pila
       void *t; 
       // creamos un nuevo nodo
       struct my_stack_node *p=stack->top;
       // comprobamos que la pila no este vacia
       if(p){
        t = p->data;
        // el top pasa a seguir el siguiente elemento
        stack->top=p->next;
        // eliminamos el espacio que ocupa el nodo que se extare de la pila
        free(p);
        

       }
       // devolvemos el puntero con el elemnto designado
       return t;
       

};
int my_stack_len(struct my_stack *stack){
       int i=0;
       // comprobamos que el stack no esté vacío.
       if(stack->top!=NULL){
              // creamos un nodo que copia a stack
              struct my_stack_node *p=stack->top;
              // hacemos un bucle que nos recorre toda la pila y almacena su longitud en la variable i previamente aisgnada
              while(p!=NULL){
                     i++;
                     p=p->next;
              }
       // devolvemos la longitud de la pila
       return i;
       }

}

int my_stack_purge(struct my_stack *stack){
       int bytes=0;
       // hacemos un bucle para recorrer la pila liberando asi toda la memoria
       while(stack->top){
              // creamos un nodo que copia a stack
              struct my_stack_node *p=stack->top;
              // pasamos al siguiente elemento
              stack->top=p->next;
              bytes=bytes+sizeof(*p);
              bytes=bytes+stack->size;
              // liberamos parte de la memoria
              free(p);
       }
       bytes+=sizeof(*stack);
       // liberamos la memoria de la pila
       free(stack);
       // devolvemos en bytes la memoria liberada
       return bytes;

}

struct my_stack *my_stack_read(char *filename){
       // abrimos el fichero en modo lectura
       int fd=open(filename,O_RDONLY);
       // comprobamos que el fichero no este vacio
       if(fd==-1){
              return NULL;
       }
       int size;
       if(read(fd, &size, sizeof(int))<0){
              return NULL;
       }
       struct my_stack *s = my_stack_init(size);
       // inicializamos un puntero que almacena los datos leidos
       void *buf;
       buf=malloc(size);
       // hacemos un bucle que nos lee el fichero hasta que no quede nada en el fichero
       while(read(fd,buf,size)!=0){
              my_stack_push(s,buf);
              buf=malloc(size);
       }

       // cerramos el fichero
       close(fd);
       // devolvemos la pila
       return s;

}

int my_stack_write(struct my_stack *stack, char *filename){
       // abrimos el fichero y le damos permisos
       int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC ,S_IRUSR | S_IWUSR);
       // comprobamos que el fichero no este vacio
       if (fd == -1) {
              return -1;
       }
       struct my_stack *s;
       s=my_stack_init(stack->size);
       // creamos un nodo copiando a stack
       struct my_stack_node *p=stack->top;
       // Modificamos la pila para leerla en sentido contrario 
       for (int i = 0; i < my_stack_len(stack); i++){
              my_stack_push(s, p->data);
       
       p = p->next;
       }
       // inicializamos una variable para contar la cantidad de elementos escritos
       int elementos=0;
       write(fd, &stack->size, sizeof(int));
       // mientras exista el stack s continuamos escribiendo en el fichero 
       while (s->top){
              if(!write(fd, my_stack_pop(s), s->size)){
                     return -1;
              }
       elementos++;
       }
       // cerramos el fichero
       close(fd);
       // devolvemos la cantidad de datos que hemos escrito
       return elementos;

}