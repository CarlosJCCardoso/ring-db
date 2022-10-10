// tcp_sc.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "tcp_sc.h"

// TCP funções server e client
// Funções ligadas ao setup de sockets tcp
// Funções ligadas a comunicações realizadas por tcp


// Retorna o fd do socket em caso de sucesso
// Retorna -1 caso contrário
int openTCPserver(char IP[],char port[]){
    int tcp_fd = -1;
    int errcode;
    struct addrinfo hints, *res;

    // Abrir servidor TCP
    if((tcp_fd = socket(AF_INET,SOCK_STREAM,0))==-1){
        printf("Erro a abrir socket\n"); // Delete
        return -1;
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_STREAM;//TCP socket
    hints.ai_flags=AI_PASSIVE;

    if((errcode=getaddrinfo( IP, port ,&hints,&res))!=0){
         printf("Erro getaddrinfo\n");
         return -1;
    }

    // Bind
    if(bind(tcp_fd, res->ai_addr,res->ai_addrlen)==-1){
        printf("Erro bind\n");
        return -1;//error
    }

    // Listen
    if(listen(tcp_fd, 5)==-1){
        printf("Erro listen\n");
        return -1;
    }

    return tcp_fd;
}


int TCPconnect(char IP[], char port[]){
    int fd=-1;
    int n = 0;
    struct addrinfo hints, *res;

    fd = socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if(fd == -1) 
        return -1;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    
    
    n = getaddrinfo(IP, port , &hints, &res);

    if(n!=0){/*error*/
        printf("Erro getaddrinfo\n");
        return -1;
    }
    n = connect(fd, res->ai_addr,res->ai_addrlen);
    if(n==-1){/*error*/
        printf("Erro connect\n");
        return -1;
    }

    return fd;
}

void tcpWrite(int destination_fd, char message[]){
    char*ptr, buffer[MAXLINE];
    int nbytes, nleft, nwritten;
    
    ptr = strcpy(buffer, message);
    nbytes = strlen(message) + 1;
    nleft=nbytes;
    
    while(nleft>0){
        nwritten = write(destination_fd, ptr, nleft);
        if(nwritten<=0){
            printf("Erro write\n");
            exit(1);
        }
        nleft-=nwritten;
        ptr+=nwritten;
    }
    return;
}
