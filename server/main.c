#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include "fs/operations.h"
#include <sys/times.h>

/////
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include "tecnicofs-api-constants.h"
////

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
/* !!! Proj 3 !!! */
#define INDIM 30
#define OUTDIM 512

/* * * * * * * * * * * * * * * * * * * * * * * * *
+                                               +
+   Projeto de Sistemas Operativos 2020-21      +
+   Segundo enunciado LEIC-A                    +
+   Turno: Ter 15:30 17:00 LAB13                +
+   Grupo 42                                    +
+       89451 - Guilherme Areias                +
+                                               +
* * * * * * * * * * * * * * * * * * * * * * * * */

int numberThreads = 1;
pthread_mutex_t trinco;
/* UDP Server variables */
int sockfd;
struct sockaddr_un server_addr;
socklen_t addrlen;
char *path;


/******************************************/
/** Funcoes incorporadas para o programa **/
/******************************************/


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/** Funcao de output de erros para averiguacao de argumentos **/

void errorInput(int flag){
    if(flag == 0){
        fprintf(stderr, "Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }
    else if(flag == 1){
        fprintf(stderr, "Error: invalid input directory\n");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(stderr, "Error: invalid value of of numthreads\n");
        exit(EXIT_FAILURE);
    }
}

/** Funcao de output de erros para funcao 'm' **/

void errorMV(){
    fprintf(stderr, "Error: wrong directory input commands\n");
    //exit(EXIT_FAILURE);
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

/** Funcao applyCommands **/
int applyCommands(char* command){

    char token;
    char name[MAX_INPUT_SIZE], type[MAX_INPUT_SIZE];
    int searchResult, a, searchResult1, searchResult2;

    int numTokens = sscanf(command, "%c %s %s", &token, name, type);
    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", command);

    switch (token) {
        case 'c':
            switch (type[0]) {
                case 'f':
                    printf("Create file: %s\n", name);
                    pthread_mutex_lock(&trinco);
                    a = create(name, T_FILE);
                    pthread_mutex_unlock(&trinco);
                    return a;
                    break;
                case 'd':
                    printf("Create directory: %s\n", name);
                    pthread_mutex_lock(&trinco);
                    a =create(name, T_DIRECTORY);
                    pthread_mutex_unlock(&trinco);
                    return a;
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            pthread_mutex_lock(&trinco);
            searchResult = lookup(name);
            if (searchResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            pthread_mutex_unlock(&trinco);
            return searchResult;
            break;
        case 'd':
            printf("Delete: %s\n", name);
            pthread_mutex_lock(&trinco);
            a = delete(name);
            pthread_mutex_unlock(&trinco);
            return a;
            break;
        case 'm':
            pthread_mutex_lock(&trinco);
            searchResult1 = lookup(name);
            searchResult2 = lookup(type);
            if(searchResult1 >= 0 && searchResult2 >= 0){
                a = move(name, type, searchResult1, searchResult2);
            }  
            else{
                fprintf(stderr, "Error: wrong directory input commands\n");
                a = -1;
            }
            pthread_mutex_unlock(&trinco);
            return a;
            break;
        case 'p':
            printf("Printed to: %s\n", name);
            pthread_mutex_lock(&trinco);
            /*
            stdout = fopen(name, "w");
            print_tecnicofs_tree(stdout);
            fclose(stdout);
            */
            FILE *fp = fopen(name, "w");
            print_tecnicofs_tree(fp);
            fclose(fp);

            pthread_mutex_unlock(&trinco);
            return 1;
            break;
        case 's':
            printf("Connected to %s\n", name);
            return 1;
            break;
        default: { // error
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    return -3;
}

/** Funcao associada a uma thread que executa applyCommands() **/

void *fnApply(void* argv) {

    /* Usar o ciclo while do main aqui 
    para cada thread, ou seja, cada thread vai ficar 'a
    espera de receber uma mensagem de um cliente qualquer */

    while (1) {
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c, a;
        char buffer[1024];

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);

        if (c > 0){
            //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
            in_buffer[c]='\0';
            printf("Recebeu mensagem de %s:\n", client_addr.sun_path);
            c = sprintf(out_buffer, "Received: %s\n", in_buffer);
            a = applyCommands(in_buffer);
            sprintf(buffer,"%d", a);
            sendto(sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&client_addr, addrlen);
        }

    }

    return NULL;
}

/*****************/
/** Funcao Main **/
/*****************/

int main(int argc, char* argv[]) {

    int i = 0;

    /* init filesystem */
    init_fs();

    /* UDP Server */
    if (argc < 2)
        exit(EXIT_FAILURE);

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv[2];

    unlink(path);

    addrlen = setSockAddrUn (argv[2], &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    /* inicializacao dos trinco */
    pthread_mutex_init(&trinco, NULL);

    numberThreads = atoi(argv[1]);
    pthread_t tid[numberThreads];
    /* Criacao das threads */
    for (i = 0; i < numberThreads; i++) {
        if (pthread_create (&tid[i], NULL, fnApply, (void*)&argv) != 0){
            exit(EXIT_FAILURE);
        }
    }

    /* Eleminacao das threads */
    for (i = 0; i < numberThreads; i++) {
        pthread_join(tid[i], NULL);
    }

    /* Eliminacao dos trinco e rwlock */
    pthread_mutex_destroy(&trinco);

    /* Fechar e apagar o nome do socket, apesar deste programa 
    nunca chegar a este ponto */
    close(sockfd);
    unlink(argv[2]);

    /* Fim */
    exit(EXIT_SUCCESS);
}
