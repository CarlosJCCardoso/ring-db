#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>




#include "udp_sc.h"

// UDP funções server e client
// udp_sc.c
// Fortemente baseado em: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
// Funções ligadas ao setup de sockets udp
// Funções ligadas a comunicações realizadas por udp

// Retorna o fd do socket UDP em caso de sucesso
// Retorna -1 caso contrário
int setup_UDP_socket(char IP[], char port[]){
    int udp_fd; // socket file descriptor
    int int_port;
    struct sockaddr_in servaddr;

    int_port = atoi(port);
    if ( (int_port <= 0) || (int_port > 65535) ) {
        printf("Invalid port number");
        return -1;
    } 
       
    // Creating socket file descriptor
    if ( (udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return -1;
    }
       
    memset(&servaddr, 0, sizeof(servaddr));
       
    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_port = htons(int_port);
    inet_aton(IP, &servaddr.sin_addr);
       
    // Bind the socket with the server address
    if ( bind(udp_fd, (const struct sockaddr *)&servaddr, 
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        return -1;
    }

    return udp_fd;
}


void udpBasicSend(int destination_fd, char destination_IP[], char destination_port[], char message[]){
    int n;
    int lenght = strlen(message); 
    
    struct addrinfo hints,*res;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;//IPv4
    hints.ai_socktype = SOCK_DGRAM;//UDP socket
    
    n = getaddrinfo(destination_IP, destination_port, &hints, &res);
    if(n!=0){/*error*/
        printf("Erro getaddrinfo\n");
        return;
    }
    
    
    n = sendto(destination_fd, message , lenght , 0 , res->ai_addr, res->ai_addrlen);
    if(n==-1){
        //printf("Failed to send %s via udp\n", message);
    }  
    return;
    
}



//////////////////////////////////////////////////////////////////////////////////7

//socklen_t ai_addrlen; // address length (bytes)
//struct sockaddr *ai_addr; // socket address
int udpSend(int destination_fd, char destination_IP[], char destination_port[], char message[]){
    int n, m, read, tries=5;
    
    char checkOriginIP[128], checkOriginPort[128];
    int origin = 0; // if(checkOriginIP == destination_IP && checkOriginPort == destination_port)
    int done = 0;
    
    
    int lenght = strlen(message);
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct addrinfo hints,*res;
    char resp[10]="\0";
    fd_set rfdsACK;
    struct timeval timeout;
    timeout.tv_sec = 0; //Seconds
    timeout.tv_usec = 200000; //Micro seconds
    

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket
    
    n = getaddrinfo(destination_IP, destination_port, &hints, &res); 
    if(n!=0){/*error*/
        printf("Erro getaddrinfo\n");
        return -1;
    }

    do{
        n = sendto(destination_fd, message , lenght , 0 , res->ai_addr, res->ai_addrlen);
        if(n==-1){
            //printf("Failed to send %s via udp\n", message);
        }
        FD_ZERO(&rfdsACK);
        FD_SET(destination_fd, &rfdsACK);
        read = select(destination_fd+1, &rfdsACK, (fd_set*)NULL,(fd_set*)NULL, &timeout);
        if(read!=0){
            m = recvfrom(destination_fd, resp, 10, 0, (struct sockaddr*) &addr, &addrlen);
            resp[m] = '\0';
            
            getIPPORT(checkOriginIP,checkOriginPort, (struct sockaddr*) &addr);
            if( (strcmp(checkOriginIP, destination_IP) == 0) && (strcmp(checkOriginPort, destination_port) == 0))
                origin = 1;
            else
                origin = 0;
                
            if((origin == 1) && (strcmp(resp,"ACK") == 0))
                done = 1;
            else 
                done = 0;

            
            //printf("Received udp message: %s from \n", resp);
            //print_ipv4((struct sockaddr*) &addr);
        }
        tries--;
    }while( (tries > 0) && (done!=1));
    
    if(strcmp(resp,"ACK")!=0){
        //printf("ERRO: Não foram recebidos ACKS (Enviar via TCP se possivel)\n");
        return 0;
    }
    else
    {
        //printf("ACK Recebido\n");
        return 1;
    }
}



// Recebe mensagem udp e guarda o endereço IP e porto de quem enviou para posteriormente poder retornar mensagens
int udpReceive(int fd,  char buffer[], struct sockaddr *addr ){
    int n=0;
    socklen_t addrlen;
    addrlen = sizeof(*addr);
    n = recvfrom(fd, buffer, 128 , 0 , &(*addr), &addrlen);
    
    buffer[n] = '\0';
    //printf("Received udp message: %s from \n", buffer);
    //print_ipv4(&(*addr));
    
    return n;


}

// https://stackoverflow.com/questions/37721310/c-how-to-get-ip-and-port-from-struct-sockaddr
void print_ipv4(struct sockaddr *s){
    struct sockaddr_in *sin = (struct sockaddr_in *)s;
    char ip[INET_ADDRSTRLEN];
    uint16_t port;

    //inet_ntop(AF_INET, sin->sin_addr.s_addr, ip, INET_ADDRSTRLEN)
    
    inet_ntop(AF_INET, &((*sin).sin_addr), ip, INET_ADDRSTRLEN);

    port = htons (sin->sin_port);

    printf ("host %s:%d\n", ip, port);
}


void getIPPORT(char IP[], char port[], struct sockaddr *s){
    struct sockaddr_in *sin = (struct sockaddr_in *)s;
    uint16_t int_port;
      
    inet_ntop(AF_INET, &((*sin).sin_addr), IP, INET_ADDRSTRLEN);
    
    int_port = htons (sin->sin_port);
    
    sprintf(port,"%d",int_port);
    
    return;

}


