#include <stdio.h>
#include <stdbool.h>
#include <glib.h>
#include <assert.h>
#include "command.h"


struct scommand_s{
    GList *args;
    char *redir_in;
    char *redir_out;
};

struct pipeline_s{
  GList *scommands;
  bool wait;
};

scommand scommand_new(void) {
    // Asignamos la memoria
    scommand cmd = malloc(sizeof(struct scommand_s));
    assert (cmd != NULL); // Verifica que el SO nos dió la memoria
    // Inicializamos los valores (lo dejamos limpio)
    cmd -> args = NULL;
    cmd -> redir_in = NULL;
    cmd -> redir_out = NULL;
    //Verificamos el contrato
    assert(scommand_is_empty(cmd));
    assert(scommand_get_redir_in(cmd) == NULL);
    assert(scommand_get_redir_out(cmd) == NULL);
    // Retornamos el comando listo
    return cmd;
}

scommand scommand_destroy (scommand self ) { 
 assert (self != NULL);
 // Liberamos los comandos y los nodos
 g_list_free_full(self->args, free);
 // Verificamos si redir_in y redir_out son nulos
 if (self !=NULL) {
    free(self->redir_in);
    free(self->redir_out);
    free(self);
 }
 return self;  
}

void scommand_push_back(scommand self, char * argument) {
    // Aseguramos el require
    assert (self != NULL);
    assert (argument != NULL);
    // Conectamos argument a la estructura interna
    self -> args = g_list_append(self->args, argument); // toma la lista actual (!=NULL) y le agregamos argument
    // g_list crea el 1° nodo y devuelve el nuevo puntero al inicio de la lista
    // Aseguramos ensurance
    assert (!scommand_is_empty(self));
}

void scommand_pop_front(scommand self){
    //Verificamos con assert que self y scommand_is_empty no sean NULL
    assert(self != NULL && !scommand_is_empty(self));
    //Creamos front_argument que es un char puntero y le asignamos data de args.
    char * front_argument = (char *) self->args-> data;
    //Eliminamos el primer dato del scommand
    self->args = g_list_delete_link(self->args, self->args);
    //Liberamos front_argument
    free(front_argument);

    return;    
}

void scommand_set_redir_in(scommand self, char * filename){
    assert(self != NULL);
    // Creamos una variable temporal para la dirección del primer nodo
    self->redir_in = filename;
}

void scommand_set_redir_out(scommand self, char * filename){
    assert(self!=NULL);
    // pregunta si habia un filename
    if(self->redir_out!=NULL){ 
        free(self->redir_out);
    }
    //si hay un filename libera esa memoria
    self->redir_out = filename; 
}

bool scommand_is_empty(const scommand self){
    assert(self!=NULL);
    // Analizamos la cantidad de comandos
    if (g_list_length(self->args)>0){ 
        return false; // si hay algo no es vacio
    } else {
        return true; //si es vacio devuelve true
    }
}

unsigned int scommand_length (const  scommand self) { 
    assert (self != NULL);
    // Inicializamos el contador de comandos en una variable temporal
    unsigned int largo = 0;
    // Creamos un puntero a los comandos
    GList *current = self -> args;
    while (current != NULL) {
        largo++;
        current = current -> next; // Pasamos al siguiente comando
    }
    return largo;
}

char * scommand_front(const scommand self) {
    // Verificamos requires
    assert (self != NULL && !scommand_is_empty(self)); 
    // Extraemos el comando de adelante de la secuencia
    char *down_argument = (char *) self->args->data ; 
    return down_argument;
}

char * scommand_get_redir_in(const scommand self){
    // Verificamos require
    assert(self != NULL);
    return (self->redir_in);
}

char * scommand_get_redir_out(const scommand self){
    // Verificamos require
    assert(self != NULL);
    return (self->redir_out);
}

char * scommand_to_string(const scommand self){
    assert(self != NULL);
    if(scommand_is_empty(self) && self->redir_in == NULL && self->redir_out == NULL){
        //creamos una lsita vacia con espacio dinamico y lo devolvemos
        char *resul = malloc(1 * sizeof(char));
        assert (resul != NULL);
        // Devolvemos en la posición 0 el caracter \0 que indica el final de una cadena de texto
        resul[0] = '\0';
        return resul;
    } 
    //creamos una string dinamica
    GString *resultado = g_string_new(" ");

    GList *current = self -> args;
    while(current != NULL){
        //Aca sacamos el dato y lo ponemos en arg, despues con g_string_append concatenamos al string resultado el elemento de arg
        char *arg = (char *) current->data;
        g_string_append(resultado, arg);
        //si hay mas argumentos agregamos espacio
        if(current->next != NULL || self->redir_in != NULL || self->redir_out != NULL){
            g_string_append(resultado, " ");
        }
        // Pasamos al siguiente argumento
        current = current->next;
    }
    //Si tenemos redirrecion de entrada hacemos esto:
    if(self-> redir_in != NULL){
        g_string_append(resultado, "<"); // Imprimimos el caracter < 
        g_string_append(resultado, self->redir_in); // A resultado le agregamos redir_in
        if(self->redir_out != NULL){
            g_string_append(resultado, " "); // Dejamos un espacio
        }
    }
    //Si tenemos rediredccion de salida hacemo lo mismo
    if(self->redir_out != NULL){
        g_string_append(resultado, ">");
        g_string_append(resultado, self->redir_out);
        if(self->redir_in != NULL){
            g_string_append(resultado, " ");
        }
    }
    // Creamos result para devolverlo con return y liberamos resultado
    char *result = g_string_free(resultado, FALSE);
    assert(scommand_is_empty(self) || self->redir_in == NULL || self->redir_out == NULL || strlen(result) > 0);
    return result; 
}

pipeline pipeline_new(void){
    // Asignamos la memoria
    pipeline result = malloc(sizeof(struct pipeline_s));
    // Verificamos que recibimos la memoria
    assert (result != NULL);
    // Inicializamos las variables del struct
    result -> scommands = NULL;
    result -> wait = true;
    // Verificamos ensures
    assert (result != NULL);
    assert (pipeline_is_empty(result));
    assert (pipeline_get_wait(result));

    return result;
}

pipeline pipeline_destroy(pipeline self){
    assert(self != NULL);
    // Definimos el puntero temporal current de tipo GList con la dirección de los nodos con sus comandos
    GList *current = self -> scommands;
    while(current != NULL){
        scommand_destroy(current->data); // Liberamos el comando dentro del nodo
        current = current -> next; // Pasamos al siguiente comando
    }
    g_list_free(self->scommands); // Eliminamos el nodo 
    free(self); // Si quedó algo se libera
    return NULL;
}

void pipeline_push_back(pipeline self, scommand sc){
    assert (self != NULL);
    assert (sc != NULL);
    // Agregamos el comando sc a la lista interna del pipeline
    self -> scommands = g_list_append (self->scommands, sc);
    // Aseguramos ensure
    assert (!pipeline_is_empty(self));
}

void pipeline_pop_front(pipeline self){
    assert (self!=NULL);
    assert (!pipeline_is_empty(self));
    // Guardameos el primer comando en una variable temporal
    scommand first_command = self -> scommands -> data;
    // Eliminamos el comando
    self -> scommands = g_list_remove(self->scommands, first_command);
    scommand_destroy (first_command);
}

void pipeline_set_wait(pipeline self, const bool w){
    assert (self != NULL);
    // Modificamos el valor 
    self -> wait = w;
}

bool pipeline_is_empty(const pipeline self){
    assert (self != NULL);
    // Verificar que este vacio
    return (g_list_length(self -> scommands) == 0);
}

unsigned int pipeline_length(const pipeline self){
    assert (self != NULL);
    // Asignamos una variable temporal para que guarde el largo
    unsigned int largo = g_list_length(self -> scommands);
    // Verificamos ensures
    assert ((largo == 0) == pipeline_is_empty(self));

    return largo;
}

scommand pipeline_front(const pipeline self){
    assert (self!=NULL);
    assert (!pipeline_is_empty(self));
    // Variable temporal para guardar el primer comando
    scommand primer_c = self -> scommands -> data;
    // Verificamos ensures
    assert (primer_c != NULL);
    
    return primer_c;
}

bool pipeline_get_wait(const pipeline self){
    assert (self != NULL);
    // Devolvemos el valor de la variable wait
    return (self -> wait);
}

char * pipeline_to_string(const pipeline self){
    assert(self != NULL);
    // Definimos el puntero resultado de tipo GString y le asignamos el string dinamico
    GString *resultado = g_string_new("");
    // Definimos el puntero current de tipo GList y le asignamos el comando
    GList *current = self->scommands;

    while (current != NULL) {
        scommand cmd = (scommand) current->data; // Creamos cmd y le asignamos la data 
        char *cmd_str = scommand_to_string(cmd); // Obtenemos la representación de la cadena del scommand actual y se lo asignamos a cmd_str
        g_string_append(resultado, cmd_str); // Concatenamos el comando
        free(cmd_str); // Liberamos la cadena devuelta por scommand_to_string
        if (current->next != NULL) {
            g_string_append(resultado, " | "); // Si hay más comandos en el pipeline imprimimos un pipe
        }
        current = current -> next;
    }
    if (!pipeline_get_wait(self)) {
        // Si pipeline_get_wait es false agregamos los ampersen
        g_string_append(resultado, "&"); //  Se coloca un solo & al final de un comando para que se ejecute en background sin bloquear la terminal
    }
    char *result = g_string_free(resultado, FALSE); // Liberamos resultado
    // Aseguramos ensure
    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result) > 0);

    return result;
}