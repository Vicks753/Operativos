#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {
    /* Devuelve NULL cuando hay un error de parseo */
    // Inicialización: Creamos un comando simple vacío
    scommand cmd = scommand_new();
    // Ciclo de lectura //
    arg_kind_t type;
    char *arg = NULL;
    arg = parser_next_argument(p, &type); // Extraemos los argumentos del Parser
    // Dependiendo el "input" del usuario realizamos la función correspondiente:
    while (arg != NULL) {
        //printf("arg %s\n", arg);
        if (type == ARG_NORMAL){
            scommand_push_back(cmd, arg); // Agregamos el comando a la cadena
        }

        if (type == ARG_INPUT) {
            scommand_set_redir_in(cmd, arg); // Redireccionamos el input del comando
        }

        if (type == ARG_OUTPUT) {
            scommand_set_redir_out(cmd, arg); // Redireccionamos el output del comando
        }

        arg = parser_next_argument(p, &type); // Actualizamos arg para no estar en bucle infinito
    }
    // Verificamos error de parseo
    if (scommand_is_empty (cmd)) {
        scommand_destroy(cmd);
        return NULL;
    } else {
        return (cmd);
    }
}

pipeline parse_pipeline(Parser p) {
    pipeline result = pipeline_new(); // Pipeline vacío a llenar
    scommand cmd = NULL;
    bool error = false, another_pipe = true;

    cmd = parse_scommand(p);
    error = (cmd==NULL); /* Comando inválido al empezar */
    while (another_pipe && !error) {
        pipeline_push_back(result, cmd); // Conectamos el comando al pipeline (result)
        parser_skip_blanks(p); // Consideramos los espacios
        parser_op_pipe(p, &another_pipe); // Verificamos si lo que sigue es un "|"
        // Si esto es así
        if (another_pipe){
            cmd = parse_scommand(p); // Actualizamos cmd (nuevos comandos)
            error = (cmd == NULL); // Actualizamos error
        } else {
            // Si lo que sigue no es |, en la siguiente vuelta del bucle se finaliza solo 
        }
    }
    /* Opcionalmente un OP_BACKGROUND al final */
    // Ahora toca verificar si lo que sigue es un &
    bool was_background = false, garbage = false;
    parser_skip_blanks(p);   /* Tolerancia a espacios posteriores */
    parser_op_background(p, &was_background);
    pipeline_set_wait(result, !was_background); // Si lo que hay después NO es un & el pipeline espera
    parser_garbage(p, &garbage); /* Consumir todo lo que hay inclusive el \n */
    /* Si hubo error, hacemos cleanup */
    if (error == true || garbage == true) {
        pipeline_destroy(result); 
        return NULL; // Ocurrio un error
    } else {
        return result; // Pipeline válido
    }
    
}