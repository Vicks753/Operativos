#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include "command.h"
#include "execute.h"
#include "parser.h"
#include "builtin.h"
static char **preparar_scommand(scommand cmd){
    //Convierte la estructura dle command en el arreglo de cadenas argv que necesia execvp
    //ARREGLO CHAR *ARGV
    int largo_scommand = scommand_length(cmd);
    //Creamos argv y pedimos memoria del largo de pipe +1
    char **argv = malloc((largo_scommand +1) * sizeof(char *)); 
    for(int i = 0; i != largo_scommand; i++){
        argv[i] = scommand_front(cmd);
        scommand_pop_front(cmd);
    }
    argv[largo_scommand] = NULL;
    //El free lo hacemos en el execute.
    return argv;
}
static int redirector(scommand cmd){
    //REDIRECTORES
    char *archivo_entrada = scommand_get_redir_in(cmd);
    char *archivo_salida = scommand_get_redir_out(cmd);
    //CASO REDIR_IN DISTINTO A NULL: "<"
    if(archivo_entrada != NULL){
        int fd_in = open(archivo_entrada, O_RDONLY);
        if(fd_in == -1){
            perror("Error al redireccionar la entrada.\n");
            return -1;
        }
        if(dup2(fd_in, STDIN_FILENO) == -1){
            perror("Error en dup2 para entrada.");
            return -1;
        }
        close(fd_in);

    }
    //CASO REDIR_OUT DISTINTO A NULL: ">"
        if(archivo_salida != NULL){
        //Abrimos el achivo de salida, usamos O_WRONLY que es flag de solo escritura, O_CREAT que crea el arhcivo si no existia y O_TRUNC que si el archivo existe pero es solo escritura, borra su contenido.
            int fd_out = open(archivo_salida,O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(fd_out == -1){
                perror("Error al redireccionar la salida.\n");
                return -1;
            }
            if(dup2(fd_out, STDOUT_FILENO) == -1){
                perror("Error en dup2 para salida.");
                return -1;
            }
            close(fd_out);
        }
    return 0;
}
static void set_pipes(int i, int pipe_lenght, int pipes[][2]){
    if(pipe_lenght == 1){
        //No hay mas comandos, hacemos return.
        return;
    }
    //Caso primer pipe.
    if(i == 0){
        //Es el pipe posicion_actua y escribe, "1", 
        dup2(pipes[i][1], STDOUT_FILENO);
    //Caso ultimo pipe.
    }else if(i == (pipe_lenght - 1)){
        dup2(pipes[i-1][0], STDIN_FILENO);
    }else{
    //Caso midle pipe.
        dup2(pipes[i][1], STDOUT_FILENO);
        dup2(pipes[i-1][0], STDIN_FILENO);
    }
    for (int j=0; j  < (pipe_lenght -1); j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    return;
}



void execute_pipeline(pipeline apipe){
    //Chequeamos el requires.
    assert(apipe != NULL);
    //Vemos el caso en el que tengmaos un unico comando interno.
    if(builtin_alone(apipe)){
        //Si lo tenemos, lo ejecutamos y hacemos return.
        builtin_run(pipeline_front(apipe));
        return;
    }
    //Definimos variables.
    int largo_pipe = pipeline_length(apipe);
    //Aca almacenamos la informacion para crear los pipes. int[2] pq pipes necesita [][] que sean 0 o 1, y necesitamos largo_pipes-1 cantidad de esos.
    int (*pipes)[2] = malloc((largo_pipe-1) * sizeof(int[2]));
    //Para guardar los pids de todos los hijos.
    int *pids = malloc(largo_pipe * sizeof(int));
    //Creamos los pipes del padre.
    for(int j = 0; j < (largo_pipe-1); j++){
        pipe(pipes[j]);
    }
    //Empezamos recorriendo desde 0 hasta el largo del pipe.
    for(int i = 0; i < largo_pipe; i++){
        //llamamos el comando del frente y avanzamos al siguiente.
        scommand cmd = pipeline_front(apipe);
        pipeline_pop_front(apipe);

        int pid = fork();
        //Chequeos del fork, vemos quien es.
        if(pid < 0){
            perror("no forkeaste.");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){
            //Llamamos a la función que crea los pipes del hijo.
            set_pipes(i,largo_pipe, pipes);
            //Llamamos a la función que maneja los redirectores.
            if(redirector(cmd) == -1){
                exit(EXIT_FAILURE);
            }
            //Preparamos el comando.
            char **argv = preparar_scommand(cmd);
            //Ejecutamos.
            if(argv[0] != NULL){
                execvp(argv[0], argv);
                perror("execvp fallo.");
            }
            //Liberamos memoria pendiente del preparar_scommand.
            free(argv);
            exit(EXIT_FAILURE);
        }
        //Caso padre (solo cerramos los pipes que ya no usamos y guardamos el pid del hijo actual).
        if(pid > 0){
            pids[i] = pid;
            if(i > 0){
            close(pipes[i-1][0]);
            close(pipes[i-1][1]);
            }
        }
    }
    //Liberamos la memoria que pedimos para pipes.
    free(pipes);
    //Veamos si tiene que esperar.
    if(pipeline_get_wait(apipe)){
        for(int n = 0; n < largo_pipe; n++){
            int status;
            waitpid(pids[n], &status, 0);
        }
    }
    //Liberamos la memoria que pedimos para el pids.
    free(pids);
    return;
}