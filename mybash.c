#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

static void show_prompt(void) {
    printf ("mybash> ");
    fflush (stdout);
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt();
        pipe = parse_pipeline(input); 
        /* pipe puede tomar dos valores por parse_pipeline
            pipe == NULL -> no hace falta evaluarlo (parse pipeline ya se encarga de eliminarlo)
            pipe != NULL 
        */

        /* Hay que salir luego de ejecutar? */
        quit = parser_at_eof(input); // Controla el final del loop
        /* Si el parser llegó al final del archivo podemos ejecutar los comandos (quit = true)*/ 
        if (pipe!=NULL) {
            execute_pipeline(pipe); // Ejecutamos el pipeline
            pipe = pipeline_destroy(pipe); // pipe queda en NULL ("puntero limpio")
        } 
    }
    parser_destroy(input); input = NULL;
    return EXIT_SUCCESS;
}

