#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

/* * * * * * * * * * * * * * * * * * * * * * * * *
+                                               +
+   Projeto de Sistemas Operativos 2020-21      +
+   Segundo enunciado LEIC-A                    +
+   Turno: Ter 15:30 17:00 LAB13                +
+   Grupo 42                                    +
+       89451 - Guilherme Areias                +
+                                               +
* * * * * * * * * * * * * * * * * * * * * * * * */

int sockfd;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
char CLIENT_BASE[50] = "/tmp/client";

int setSockAddrUn(char *path, struct sockaddr_un *addr) {
  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


int tfsCreate(char *filename, char nodeType) {
  char inputFile[MAX_INPUT_SIZE];
  char buffer[MAX_INPUT_SIZE];
  int a = -1;

  sprintf(inputFile, "c %s %c", filename, nodeType);
  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  }

  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsDelete(char *path) {
  char inputFile[MAX_INPUT_SIZE];
  char buffer[MAX_INPUT_SIZE];
  int a = -1;

  sprintf(inputFile, "d %s", path);
  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  }

  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsMove(char *from, char *to) {
  char inputFile[MAX_INPUT_SIZE];
  char buffer[MAX_INPUT_SIZE];
  int a = -1;

  sprintf(inputFile, "m %s %s", from, to);

  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  }

  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsLookup(char *path) {
  char inputFile[MAX_INPUT_SIZE];
  char buffer[MAX_INPUT_SIZE];
  int a = -1;

  sprintf(inputFile, "l %s", path);
  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  }

  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsPrint(char *path) {
  char inputFile[MAX_INPUT_SIZE];
  char buffer[MAX_INPUT_SIZE];
  int a = -1;

  sprintf(inputFile, "p %s", path);
  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  }

  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsMount(char * sockPath) {
  char buffer[1024];
  char clientID[50];
  int a = -1;
  char inputFile[1024];
  strcpy(inputFile, "s \0");

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    return -1;
  }

  sprintf(clientID,"%d", getpid());
  strcat(CLIENT_BASE, clientID);
  printf("%s\n", CLIENT_BASE);

  unlink(CLIENT_BASE);
  clilen = setSockAddrUn (CLIENT_BASE, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    return -1;
  }  

  strcat(inputFile, CLIENT_BASE);
  //printf("%s\n", inputFile);
  servlen = setSockAddrUn(sockPath, &serv_addr);
  if (sendto(sockfd, inputFile, strlen(inputFile)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return -1;
  } 
  
  a = atoi(buffer);
  if (a < 0)
    return -1;
  return 0;
}

int tfsUnmount() {
  int i = close(sockfd);
  int e = unlink(CLIENT_BASE);

  if ( (i == 0) && (e == 0))
    return 0;
  return -1;
}
