// ringv10.c
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "nodes.h"
#include "enderecos.h"
#include "comandos.h"
#include "udp_sc.h"
#include "tcp_sc.h"
#include "states.h"
#include "messages.h"

#include "ring.h"

#define MAXLINE 1024


int main(int argc, char* argv[]){
    // Definição de nós
    NODE self;       initializeNODE(&self);     // Próprio nó
    NODE chord;      initializeNODE(&chord);    // Nó correspondente ao possível chord
    NODE tempNODE;   initializeNODE(&tempNODE); // Nó temporário
    NODE extNODE;    initializeNODE(&tempNODE); // Nó temporário para guardar nó que envia EPRED 
    
    
    // Definição de conexões a nós
    CONNECTION predecessor;  initializeCONNECTION(&predecessor);  // Predecessor
    CONNECTION sucessor;     initializeCONNECTION(&sucessor);     // Sucessor
    CONNECTION tempSucessor; initializeCONNECTION(&tempSucessor); // Guardar o sucessor (nova ligação) até ser verificado se é um sucessor válido

    // Definição do socket utilizado para comunicações via udp
    int udp_fd;

    // Definição do socket utilizado para o servidor tcp
    int tcp_fd;

    // Variáveis relacionadas com o estabelecimento de ligações
    struct sockaddr addr;
    socklen_t addrlen;
    int newfd;         // fd temporário para guardar novas conexões
    int oldSucessorfd=0; // fd temporário para guardar o sucessor anterior enquanto se dá a mudança de ligações

    // Variáveis relacionadas com o select
    fd_set rfds; // Set de file descriptors a serem verificados pelo select
    int maxfd=0, counter=0;

    // Variáveis relacionadas com o processamento de mensagens
    char buffer[MAXLINE]="\0";
    char message[MAXLINE];
    //char *ptr;
    MESSAGE messageType;

    // Variáveis relacionadas com o processamento de comandos
    COMMAND userCommand;
    char commandStr[MAXLINE];

    // Variáveis relacionadas com a "máquina de estados"
    STATE prevSTATE = BOOT, currSTATE = BOOT;
    
    // Variáveis relacionadas com o processamento de pedidos de find
    int seq_n=0, find_k=0, chord_occupied=0;
    WaitList WaitList[100]; // O valor de requested_key[seq_n] corresponde à key procurada no pedido com número de sequência = seq_n
    negativeOnes(WaitList, 100); // Inicializa o vetor requested_key a -1 para posteriormente determinar que números de sequência não estão a ser utilizados
    
    //int n=0;
   
    

    // Validar e processar parâmetros de entrada passados (ring i i.IP i.port)
    if( (self = processArguments(argc, argv)).key == -1){
        printf("Invalid parameters\n"); // Delete
        exit(1);
    }

    //Interface
    printInterface();

    // Setup à socket udp e bind ao IP e porto do nó
    if( (udp_fd = setup_UDP_socket(self.IP, self.port)) == -1){
        printf("Failed to setup udp socket\n"); // Delete
        exit(1);
    }

    // Setup ao servidor TCP
    if( (tcp_fd = openTCPserver(self.IP, self.port)) == -1){
        printf("Failed to setup tcp server\n"); // Delete
        exit(1);
    }

    // Ciclo do processo
    while(1){
        // Select
        FD_ZERO(&rfds);

        // Set dos file descriptors específicos para cada estado
        switch(currSTATE){
            case BOOT:
                FD_SET(0,  &rfds);
                maxfd = 0;
            break;
            
            case ALONE:
                FD_SET(0,  &rfds);
                FD_SET(tcp_fd, &rfds); maxfd = tcp_fd;
                FD_SET(udp_fd, &rfds); maxfd = max(udp_fd,maxfd);
            break;
                
            case WAIT_EPRED:
                FD_SET(0, &rfds);
                FD_SET(udp_fd, &rfds); maxfd=udp_fd;
            break;
                
            case WAIT_SELF:
                FD_SET(0, &rfds);
                FD_SET(tcp_fd, &rfds); maxfd = tcp_fd;
                FD_SET(sucessor.fd, &rfds); maxfd = max(maxfd, sucessor.fd);
                FD_SET(tempSucessor.fd, &rfds);maxfd = max(maxfd, tempSucessor.fd);
                if(prevSTATE == CONNECTED){
                    FD_SET(oldSucessorfd, &rfds);
                    maxfd = max(maxfd, oldSucessorfd);
                }
            break;
            
            case WAIT_SUCESSOR:
                FD_SET(0, &rfds);  
                FD_SET(tcp_fd, &rfds);           maxfd = tcp_fd;
                FD_SET(predecessor.fd , &rfds);  maxfd = max(maxfd, predecessor.fd);
            break;
            
            case CONNECTED:
                FD_SET(0, &rfds);  
                FD_SET(tcp_fd, &rfds);          maxfd = tcp_fd;
                FD_SET(sucessor.fd  , &rfds);   maxfd = max(maxfd, sucessor.fd);
                FD_SET(predecessor.fd , &rfds); maxfd = max(maxfd, predecessor.fd);
                FD_SET(udp_fd, &rfds);          maxfd = max(maxfd, udp_fd);
            break;  
            
            default:
            break;
        }


        // Funcionamento do select
        counter = select(maxfd+1, &rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        if(counter<=0){/*error*/
            perror("select() error"); exit(1); 
        }

        for(;counter;--counter){
            switch(currSTATE){
//// BOOT ///////////////////////////////////////////////////
                case BOOT:
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);

                        //Processar comando do teclado
                        fgets(commandStr, 128, stdin); 
                        processCommand( commandStr , &userCommand, &tempNODE);
                        switch(userCommand){
                            case NEW:
                                changeState(&prevSTATE, &currSTATE, ALONE);
                            break;
                            case BENTRY:
                                sprintf(message, "EFND %d", self.key);
                                // Enviar EFND para nó delegado pelo cliente
                                if(udpSend(udp_fd, tempNODE.IP, tempNODE.port, message)==0){
                                    changeState(&prevSTATE, &currSTATE, BOOT);
                                }
                                else{
                                    changeState(&prevSTATE, &currSTATE, WAIT_EPRED);
                                }
                            break;
                            case PENTRY:
                                // Conectar ao predecessor
                                if((predecessor.fd = TCPconnect(tempNODE.IP, tempNODE.port)) != -1){
                                    // Guardar predecessor
                                    copyNODE(&(predecessor.node), tempNODE);
                                    
                                    // Enviar mensagem <SELF i i.IP i.port\n>
                                    sprintf(message, "SELF %d %s %s\n", self.key, self.IP, self.port);
                                    tcpWrite(predecessor.fd, message); 
                                    changeState(&prevSTATE, &currSTATE, WAIT_SUCESSOR);
                                }
                                else{
                                    printf("Pentry failed. Could not connect to predecessor\n");
                                } 
                            break;
                            case SHOW:
                                showConnections(self, sucessor.node, predecessor.node, chord);
                            break;
                               
                            case EXIT:
                                exit(1);
                            break;
                                
                            default:
                                printf("Invalid command\n");
                            break;
                        } // switch(userCommand) 

                    } //if(FD_ISSET(0, &rfds)){
                break; // BOOT
                    
//// ALONE ///////////////////////////////////////////////////
                case ALONE: 
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);
                        //Processar comando do teclado
                        fgets(commandStr, 128, stdin); 
                        processCommand(commandStr ,  &userCommand, &tempNODE);
                        switch(userCommand){
                            case EXIT:
                                printf("Exiting...\n");
                                exit(0);
                            break;
                            case LEAVE:
                                changeState(&prevSTATE, &currSTATE, BOOT);
                            break;
                            case SHOW:
                                showConnections(self, sucessor.node, predecessor.node, chord);
                            break;
                            default:
                                printf("Invalid command\n");
                            break;
                        }

                        
                    } // if(FD_ISSET(0, &rfds))
                    
                    // Recebe mensagens udp (EFND)
                    if(FD_ISSET(udp_fd, &rfds)){
                        FD_CLR(udp_fd, &rfds);
                        
                        udpReceive(udp_fd,  buffer, &addr);
                        //print_ipv4(&addr);
                        
                        getIPPORT(extNODE.IP, extNODE.port, &addr);                        
                        
                        processMessage(buffer, &messageType, &tempNODE, &seq_n, &find_k);
                       

                        if(messageType == EFND){
                            // Enviar ACK
                            udpBasicSend(udp_fd, extNODE.IP, extNODE.port,  "ACK");
                            
                            // Caso a chave pertença ao nó atual

                            // Enviar mensagem de resposta ao nó exterior (<EPRED pred pred.IP pred.port\n>)
                            sprintf(message, "EPRED %d %s %s", self.key, self.IP, self.port);
                           
                            if(udpSend(udp_fd, extNODE.IP, extNODE.port, message)==0){ 
                                //printf("Não foram recebidos ACKS (Existe um problema deligação com o nó exterior!)\n)");
                            }
                            else{
                                //printf("ACK Recebido\n");
                            }
                            
                        }
                        
                        else{
                        }
                        

                    } //if(FD_ISSET(udp_fd, &rfds)){
                            
                    // Recebe novas conexões TCP
                    if(FD_ISSET(tcp_fd, &rfds)){
                        //printf("A receber nova conexão tcp (ALONE)\n");
                        FD_CLR(tcp_fd, &rfds);
                        addrlen = sizeof(addr);
                        if((newfd = accept(tcp_fd, &addr, &addrlen))==-1){
                            printf("Erro accept\n");
                            exit(1);//error
                        }
                        
                        tempSucessor.fd = newfd;
                        
                        // Transita para o estado em que espera pela mensagem de <SELF>
                        changeState(&prevSTATE, &currSTATE, WAIT_SELF);
                        
                    } // if(FD_ISSET(tcp_fd, &rfds))
                break; // ALONE
                
//// WAIT_PRED ///////////////////////////////////////////////////

                case WAIT_EPRED:
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);
                        //Processar comando do teclado (exit ou leave?)
                        fgets(commandStr, 128, stdin); 
                        processCommand(commandStr, &userCommand, &tempNODE);
                        if(userCommand == EXIT){
                            printf("Exiting...\n");
                            exit(0);
                        }
                        else if(userCommand == SHOW){
                            showConnections(self, sucessor.node, predecessor.node, chord);
                        }
                        else {
                            printf("Invalid command\n");
                        }
                    } // if(FD_ISSET(0, &rfds))

                    // Recebeu mensagem de <EPRED> (UDP) do nó a que delegou o FND
                    if(FD_ISSET(udp_fd , &rfds)){
                        FD_CLR(udp_fd ,&rfds);
                        
                        udpReceive(udp_fd,  buffer, &addr);
                        //print_ipv4(&addr);
                        
                        //Send ACK
                        getIPPORT(extNODE.IP, extNODE.port, &addr);
                        udpBasicSend(udp_fd, extNODE.IP, extNODE.port,  "ACK");

                        processMessage(buffer, &messageType, &tempNODE, &seq_n, &find_k);
                        
                        if(messageType == EPRED){
                            // Conectar ao predecessor
                            if((predecessor.fd = TCPconnect(tempNODE.IP, tempNODE.port)) != -1){
                                // Guardar predecessor
                                copyNODE(&(predecessor.node), tempNODE);
                            
                                // Enviar mensagem <SELF i i.IP i.port\n>
                                sprintf(message, "SELF %d %s %s\n", self.key, self.IP, self.port);
                                tcpWrite(predecessor.fd, message); 
                                
                                changeState(&prevSTATE, &currSTATE, WAIT_SUCESSOR);
                            }
                            else{
                                printf("Failed to connect to predecessor\n");
                                changeState(&prevSTATE, &currSTATE, BOOT); 
                            }
                        
                        }// Caso contrário continuar à espera do EPRED

                        
                    }
                break; // WAIT_EPRED 
                     
                    
//// WAIT_SELF ///////////////////////////////////////////////////
                case WAIT_SELF:
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);
                        //Processar comando do teclado (exit ou leave?)
                        fgets(commandStr, 128, stdin); 
                        processCommand(commandStr ,  &userCommand, &tempNODE);
                        switch(userCommand){
                            case EXIT:
                                //leave
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                printf("Exiting...\n");
                                exit(0);
                            break;
                            case LEAVE:
                                //leave
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                changeState(&prevSTATE, &currSTATE, BOOT);
                                
                            break;
                            case SHOW:
                                showConnections(self, sucessor.node, predecessor.node, chord);
                            break;
                            default:
                                printf("Invalid command\n");
                            break;
                        }
                    } // if(FD_ISSET(0, &rfds))
                    
                    // Espera pela mensagem de <SELF> do sucessor para a processar 
                    if(FD_ISSET(tempSucessor.fd , &rfds)){
                        FD_CLR(tempSucessor.fd ,&rfds);
                         
                        // Processa <SELF> e guarda como sucessor (<SELF i i.IP i.port\n >)
                        switch(isCommandComplete(&tempSucessor,buffer)){ //Se não estiver completo espera pelo resto
                            case -1: //Fechou a ligação
                            break;
                            case 0: //A espera de mensagens
                            break;
                            case 1:
                                //O commando está completo         
                                processMessage(buffer, &messageType, &tempNODE, &seq_n, &find_k);
                                
                                if(messageType == SELF){                                    
                                    // Conecta-se caso o último state tenha sido o ALONE e guarda o sucessor como predecessor (N = 2)
                                    if( prevSTATE == ALONE){
                                        if((predecessor.fd = TCPconnect(tempNODE.IP, tempNODE.port)) != -1){
                                            // Guarda sucessor
                                            copyNODE(&(sucessor.node), tempNODE);
                                            sucessor.fd = tempSucessor.fd;
                                            // Guarda sucessor como predecessor
                                            copyNODE(&(predecessor.node), sucessor.node);

                                            // Enviar mensagem <SELF i i.IP i.port\n>
                                            sprintf(message, "SELF %d %s %s\n", self.key, self.IP, self.port);
                                            tcpWrite(predecessor.fd, message);
                                        }
                                        else{
                                            printf("Could not connect to predecessor\n");
                                            exit(1);
                                        }
                                    }      
                                    
                                    // prevState == CONNECTED
                                    else if(prevSTATE == CONNECTED){
                                        // Verificar se o novo sucessor está correto
                                        // Caso o novo sucessor não esteja entre o nó atual e o anterior sucessor então é uma entrada inválida

                                        if(between(self.key, sucessor.node.key, tempNODE.key)){
                                            // Guarda sucessor
                                            copyNODE(&(sucessor.node), tempNODE);
                                            //-> (N2-WAIT_SELF) Recebe e processa <SELF 3 3.ip 3.port> (já recebeu e processou em cima)
                                            //-> (N2-WAIT_SELF) Envia  <PRED 3 3.ip 3.port> para o antigo sucessor
                                            sprintf(message, "PRED %d %s %s\n", sucessor.node.key, sucessor.node.IP, sucessor.node.port);
                                            tcpWrite(oldSucessorfd, message);
                                            close(oldSucessorfd);
                                        
                                            copyNODE(&(sucessor.node), tempNODE);
                                            sucessor.fd = tempSucessor.fd;
                                            //-> (N2-WAIT_SELF) Guarda (N3) como sucessor 
                                            //-> (N2-WAIT_SELF) -> (N2-CONNECTED) (transita no fim do case)
                                        }
                                        else{
                                            // Descarta novo sucessor e fecha conexão
                                            close(tempSucessor.fd);
                                            tempSucessor.fd = -1;
                                        }   
                                    }
                                    else if(prevSTATE == WAIT_SUCESSOR){
                                        copyNODE(&(sucessor.node), tempNODE);
                                        sucessor.fd = tempSucessor.fd;
                                    }
                                    
                                        
                                    changeState(&prevSTATE, &currSTATE, CONNECTED);                                   


                                } // if(messageType == SELF){
                                // Caso contrário continuar neste estado
 
                            break; //case1
                            default:
                                exit(1);
                            break;
                        }//isCommandComplete(&predecessor,buffer)
                    } // if(FD_ISSET(sucessor.fd , &rfds))
                     
                break; // WAIT_SELF
                    
//// WAIT_SUCESSOR ///////////////////////////////////////////////////                
                // Nó espera que o seu sucessor se conecte depois de se conectar ao predecessor
                // ou depois do seu sucessor fazer leave
                case WAIT_SUCESSOR:
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);
                        //Processar comando do teclado 
                        fgets(commandStr, 128, stdin); 
                        processCommand(commandStr , &userCommand, &tempNODE);
                        switch(userCommand){
                            case EXIT:
                                //leave
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                printf("Exiting...\n");
                                exit(0);
                            break;
                            case LEAVE:
                                //leave
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                changeState(&prevSTATE, &currSTATE, BOOT);
                            break;
                            case SHOW:
                                showConnections(self, sucessor.node, predecessor.node, chord);
                            break;
                            default:
                                printf("Invalid command \n");
                            break;
                        }
                    } // if(FD_ISSET(0, &rfds))
                    
                    
                    // Verificar se o predecessor fechou a ligação
                    // Recebe mensagem do predecessor 
                    if(FD_ISSET(predecessor.fd, &rfds)){
                        switch(isCommandComplete(&predecessor,buffer)){ //Se não estiver completo espera pelo resto
                            case -1: //Fechou a ligação
                                printf("Predecessor refused to establish connection.\n");
                                close(predecessor.fd);
                                initializeCONNECTION(&predecessor);
                                
                                changeState(&prevSTATE, &currSTATE, BOOT);
                            break;
                            case 0: //A espera de mensagens
                            break;
                            case 1:
                            break;
                        }
                   
                    }
                     
                    // Recebe nova conexão TCP
                    if(FD_ISSET(tcp_fd, &rfds)){
                        FD_CLR(tcp_fd, &rfds);
                        addrlen = sizeof(addr);
                        if((newfd = accept(tcp_fd, &addr, &addrlen))==-1){
                            printf("Erro accept \n");
                            exit(1);
                        }
                        tempSucessor.fd = newfd;
                        // Transita para o estado em que espera pela mensagem de <SELF>
                        changeState(&prevSTATE, &currSTATE, WAIT_SELF);
                        
                    } // if(FD_ISSET(tcp_fd, &rfds))
                                          
                break; // WAIT_SUCESSOR
                    
//// CONNECTED ///////////////////////////////////////////////////
                case CONNECTED:
                    // Recebe comando do teclado
                    if(FD_ISSET(0, &rfds)){
                        FD_CLR(0,&rfds);
                        //Processar comando do teclado
                        fgets(commandStr, 128, stdin); 
                        processCommand(commandStr ,  &userCommand, &tempNODE);
                        switch(userCommand){
                            case EXIT:
                                //leave
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                printf("Exiting...\n");
                                exit(0);
                            break;
                            case LEAVE:
                                // Envia o novo predecessor ao seu sucessor
                                sprintf(message, "PRED %d %s %s\n", predecessor.node.key, predecessor.node.IP, predecessor.node.port);
                                tcpWrite(sucessor.fd, message);
                                
                                leaveFunction(&sucessor, &predecessor, &chord, &extNODE, WaitList);
                                
                                // Transita de estado para o inicial
                                changeState(&prevSTATE, &currSTATE, BOOT);
                            break;

                            case SHOW:
                                showConnections(self, sucessor.node, predecessor.node, chord);
                            break;
                                
                            case FIND:
                                // Verificar se a chave não pertence ao nó atual
                                if(belongs(self.key, sucessor.node.key, tempNODE.key)){
                                    printf("The key (%d) belongs to the node with key: %d at IP: %s port: %s (Self)\n ", tempNODE.key, self.key, self.IP, self.port);
                                }
                                // Caso a chave não pertença ao nó atual
                                else{
                                    // Determinar o primeiro número de sequência disponível para efetuar o novo pedido
                                    seq_n = getSeqN(WaitList);
                                    WaitList[seq_n].fd = 0; //A procura foi iniciada pela consola
                                    WaitList[seq_n].key = tempNODE.key;
                                    
                                    // Determinar se o pedido deve ser enviado através do chord ou do sucessor

                                    if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , tempNODE.key) == 0) ){
                                        // Enviar através do sucessor
                                        sprintf(message, "FND %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                        tcpWrite(sucessor.fd, message);
                                    }
                                    else{
                                        // Enviar através do chord
                                        sprintf(message, "FND %d %d %d %s %s", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                        // Enviar pelo chord
                                        if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                            sprintf(message, "FND %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                            tcpWrite(sucessor.fd, message);
                                        } 
                                    }                                                              
                                }
                            break;
                            case CHORD:
                                // Caso ainda não exista chord
                                if(chord.key == -1){
                                    // Verificar se o "novo" chord não é o self
                                    if((strcmp(tempNODE.IP, self.IP)==0) && (strcmp(tempNODE.port, self.port)==0)){
                                        printf("Can't setup chord to self\n");
                                    }
                                    else{
                                        // Verificar se IP e port são válidos
                                        if(isValidNODE(tempNODE)){
                                            // Dar setup ao chord
                                            copyNODE(&chord, tempNODE);
                                        }
                                        else{
                                            printf("Inserted invalid node\n");
                                        }
                                        
                                    }
                                }
                                // Caso já exista chord 
                                else{
                                    printf("There is already an existing chord\n");
                                    printf("(%d %s %s)\n", chord.key, chord.IP, chord.port);
                                }
                            break;
                                
                            case ECHORD:
                                // Caso ainda não exista chord
                                if(chord.key == -1){
                                    printf("There is no chord to delete\n");
                                }
                                
                                // Caso já exista chord
                                else{
                                    clearNODE(&chord);
                                }
                            break;
                            default:
                                printf("Invalid command\n");
                            break;   
                        }
                    } // if(FD_ISSET(0, &rfds))
                    
                    // Recebe novas conexões TCP (Nó quer entrar na rede)
                    if(FD_ISSET(tcp_fd, &rfds)){
                        FD_CLR(tcp_fd, &rfds);
                         
                        // Aceita nova conexão TCP
                        addrlen = sizeof(addr);
                        if((newfd = accept(tcp_fd, &addr, &addrlen))==-1){
                            printf("Erro accept\n");
                            exit(1);//error
                        }
                         
                        // Guarda o fd do antigo sucessor temporariamente
                        oldSucessorfd = sucessor.fd;
                       
                        // Guarda o fd do possível novo sucessor
                        tempSucessor.fd = newfd;
                                                   
                        // Espera pela mensagem de <SELF> do seu novo sucessor (transita para o WAIT_SELF)
                        changeState(&prevSTATE, &currSTATE, WAIT_SELF);
                        
    
                    } // if(FD_ISSET(tcp_fd, &rfds))
                    
                    // Recebe mensagens TCP do Predecessor
                    if(FD_ISSET(predecessor.fd, &rfds)){
                        FD_CLR(predecessor.fd , &rfds);
                        switch(isCommandComplete(&predecessor,buffer)){ //Se não estiver completo espera pelo resto
                            case -1: //Fechou a ligação
                            break;
                            case 0: //A espera de mensagens
                            break;
                            case 1:
                                processMessage(buffer, &messageType, &tempNODE, &seq_n, &find_k);
                                switch(messageType){
                                    // Caso receba PRED do seu predecessor deve ligar-se ao novo predecessor 
                                    // indicado na mensagem
                                    case PRED:
                                        // Fecha ligação com o predecessor
                                        close(predecessor.fd);
                                        predecessor.fd = -1;   
                                        
                                        // Verificar se o novo predecessor é ele mesmo
                                        if(compareNODE(tempNODE, self)){
                                            changeState(&prevSTATE, &currSTATE, ALONE);
                                            
                                            clearNODE(&sucessor.node);
                                            clearNODE(&predecessor.node);
                                            
                                        }
                                        else{
                                            // Liga-se ao novo predecessor
                                            if((predecessor.fd = TCPconnect(tempNODE.IP, tempNODE.port)) != -1){
                                                // Guarda novo predecessor
                                                copyNODE(&(predecessor.node), tempNODE);
                                                // Enviar mensagem <SELF i i.IP i.port\n>
                                                sprintf(message, "SELF %d %s %s\n", self.key, self.IP, self.port);
                                                tcpWrite(predecessor.fd, message);
                                            }
                                            else{
                                                printf("Could not connect to new predecessor\n");
                                                exit(1);
                                            }                         
                                        }

                                    break;
                                    case FND:
                                        // Verificar se a chave não pertence ao nó atual
		                        if(belongs(self.key, sucessor.node.key, find_k)){ 
                                            if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
		                                // Enviar mensagem de resposta ao sucessor (<RSP k n i i.IP i.port\n >)
		                                // Onde k = key do nó que enviou o FND e i o self
		                                sprintf(message, "RSP %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
		                                tcpWrite(sucessor.fd, message);
		                            }
		                            else{
		                                sprintf(message, "RSP %d %d %d %s %s", tempNODE.key, seq_n, self.key, self.IP, self.port);
		                                
		                                if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                                    sprintf(message, "RSP %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                                    tcpWrite(sucessor.fd, message);
                                                } 
  
		                                //chord_occupied = 1;
		                            }
		                        }

                                        else{ 
                                            if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){  
                                                // Enviar pelo sucessor
                                                sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                tcpWrite(sucessor.fd, message);
                                            }
                                            // Caso contrário a mensagem deve seguir via chord
                                            else{
                                                sprintf(message, "FND %d %d %d %s %s", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                // Enviar pelo chord  
                                                if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                                    sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                    
                                                    tcpWrite(sucessor.fd, message);
                                                } 
                                            }
 
                                        }
                                        
                                    break; // case FND
                                    case RSP:
                                        // Se k for a própria chave recebemos a resposta ao nosso find
                                        if(find_k == self.key){
                                            //  Caso o pedido tenha vindo da consola
                                            if(WaitList[seq_n].fd==0){ 
                                                // Retorna a chave, o endereço IP e o porto do nó aos quais a chave pertence.
                                                printf("The key (%d) belongs to the node with key: %d at IP: %s port: %s\n", WaitList[seq_n].key ,tempNODE.key, tempNODE.IP, tempNODE.port);
                                                WaitList[seq_n].key = -1;
                                                WaitList[seq_n].fd = -1;
                                            }
                                            // Caso o pedido tenha vindo de um nó exterior (bentry)
                                            // (Recebemos a RSP ao pedido feito por um nó exterior ao nosso nó)
                                            else if(WaitList[seq_n].key!=-1){
                                                //  Enviar mensagem de resposta ao nó exterior (<EPRED pred pred.IP pred.port\n>)
                                                sprintf(message, "EPRED %d %s %s", tempNODE.key, tempNODE.IP, tempNODE.port);
                                                
// Tem de ser o extNODE associado ao pedido seq_n 
                                                // udpSend(udp_fd, extNODE.IP, extNODE.port, message); // Tem de ser o extNODE associado ao pedido seq_n
                                                if(udpSend(udp_fd, WaitList[seq_n].IP, WaitList[seq_n].port, message)==0){ 
                                                } 
                                                
                                                // Limpar o pedido da lista
                                                WaitList[seq_n].key = -1;
                                                WaitList[seq_n].fd = -1;
                                                strcpy(WaitList[seq_n].IP, "\0");
                                                strcpy(WaitList[seq_n].port, "\0");

                                            }
                                        }
                                        //  Caso a chave não pertença ao nó atual
                                        else{
                                            // Determinar se o pedido deve ser enviado através do chord ou do sucessor
                                            if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
                                                // Enviar através do sucessor
                                                sprintf(message, "RSP %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                tcpWrite(sucessor.fd, message);
                                            
                                            }
                                            else{
                                                // Enviar através do chord
                                                sprintf(message, "RSP %d %d %d %s %s", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                
                                                // Enviar pelo chord                                                
                                                if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                                    sprintf(message, "RSP %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                                    
                                                    tcpWrite(sucessor.fd, message);
                                                }

                                            
                                            }
                                        }
                                    break; // case RSP
                                    
                                    default:
                                    break; // default
                                
                                } // switch(messageType){
                                
                            break;
                        } // switch(isCommandComplete(&predecessor,buffer)){ 
                    } // if(FD_ISSET(predecessor.fd, &rfds)){
                    
                    
                    // Recebe mensagens TCP do Sucessor // Não é suposto receber mensagens tcp do sucessor (só vai servir para ver quando o sucessor terminou o processo abruptamente)
                    if(FD_ISSET(sucessor.fd , &rfds)){
                        FD_CLR(sucessor.fd ,&rfds);
                        switch(isCommandComplete(&sucessor,buffer)){ //Se não estiver completo espera pelo resto
                            case -1: //Fechou a ligação
                                changeState(&prevSTATE, &currSTATE, WAIT_SUCESSOR);
                            break;
                            case 0: //A espera de mensagens
                            break;
                            case 1:
                            break;
                            default:
                                exit(1);
                            break;
                        }
                    } // if(FD_ISSET(sucessor.fd, &rfds))
                    

                    // Recebe mensagem via udp (EFND, ACK, FND, RSP)
                    if(FD_ISSET(udp_fd, &rfds)){
                        FD_CLR(udp_fd, &rfds);
                        addrlen = sizeof(addr);
                        
                        udpReceive(udp_fd,  buffer, &addr);
                        //print_ipv4(&addr);
                        
                        processMessage(buffer, &messageType, &tempNODE, &seq_n, &find_k);
                        switch(messageType){
                            case ACK:
                                // Verificar se o ACK veio do chord, do extNODE ou doutro nó
                                getIPPORT(tempNODE.IP, tempNODE.port, &addr);
                                
                                // Verificar se o ACK veio do chord
                                if( (strcmp(tempNODE.IP, chord.IP) == 0) && (strcmp(tempNODE.port,chord.port) == 0) ){
                                    chord_occupied = 0;
                                }
                                else if( (strcmp(tempNODE.IP, extNODE.IP) == 0) && (strcmp(tempNODE.port, extNODE.port) == 0) ){
                                }
                                else{
                                }
                                
                            break;
                            case FND:
                                // Enviar ACK
                                getIPPORT(extNODE.IP, extNODE.port, &addr);
                                udpBasicSend(udp_fd, extNODE.IP, extNODE.port,  "ACK");
                                
                                // FND normal
                                // Verificar se a chave não pertence ao nó atual
                                if(belongs(self.key, sucessor.node.key, find_k)){ 
                                    if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
                                        // Enviar mensagem de resposta ao sucessor (<RSP k n i i.IP i.port\n >)
                                        sprintf(message, "RSP %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                        tcpWrite(sucessor.fd, message);
                                    }
                                    else{
                                        sprintf(message, "RSP %d %d %d %s %s", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                        // Enviar pelo chord
                                        
                                        if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                            sprintf(message, "RSP %d %d %d %s %s\n", tempNODE.key, seq_n, self.key, self.IP, self.port);
                                            tcpWrite(sucessor.fd, message);
                                        }
                                    }
                                }
                                        
                                else{                                    
                                    // Determinar se o pedido deve ser enviado através do chord ou do sucessor
                                    if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
                                        // Enviar pelo sucessor
                                        sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                        tcpWrite(sucessor.fd, message);

                                    }
                                    // Caso contrário a mensagem deve seguir via chord
                                    else{
                                        sprintf(message, "FND %d %d %d %s %s", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                        // Enviar pelo chord
                                        if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                            sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                            
                                            tcpWrite(sucessor.fd, message);
                                        }
                                        
                                    }

                                }
                                
                            break; // case FND

                            case RSP:
                                // Enviar ACK
                                getIPPORT(extNODE.IP, extNODE.port, &addr);
                                udpBasicSend(udp_fd, extNODE.IP, extNODE.port,  "ACK");
                                
                                // Se k for a própria chave recebemos a resposta ao nosso find
                                if(find_k == self.key){
                                    //  Caso o pedido tenha vindo da consola
                                    if(WaitList[seq_n].key==0){ 
                                        // Retorna a chave, o endereço IP e o porto do nó aos quais a chave pertence.
                                        printf("The key (%d) belongs to the node with key: %d at IP: %s port: %s\n", WaitList[seq_n].key ,tempNODE.key, tempNODE.IP, tempNODE.port);
                                        WaitList[seq_n].key = -1;
                                        WaitList[seq_n].fd = -1;
                                    }
                                    // Caso o pedido tenha vindo de um nó exterior (bentry)
                                    // (Recebemos a RSP ao pedido feito por um nó exterior ao nosso nó)
                                    else if(WaitList[seq_n].key!=-1){
                                        //  Enviar mensagem de resposta ao nó exterior (<EPRED pred pred.IP pred.port\n>)
                                        sprintf(message, "EPRED %d %s %s", tempNODE.key, tempNODE.IP, tempNODE.port);
                                        
                                        if(udpSend(udp_fd, WaitList[seq_n].IP, WaitList[seq_n].port, message)==0){ 
                                        } 
                                        
                                        // Limpar o pedido da lista
                                        WaitList[seq_n].key = -1;
                                        WaitList[seq_n].fd = -1;
                                        
                                        strcpy(WaitList[seq_n].IP, "\0");
                                        strcpy(WaitList[seq_n].port, "\0");

                                    }
                                }
                                //  Caso a chave não pertença ao nó atual
                                else{
                                    // Determinar se o pedido deve ser enviado através do chord ou do sucessor
                                    if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
                                        // Enviar através do sucessor
                                        sprintf(message, "RSP %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                        tcpWrite(sucessor.fd, message);
                                    
                                    }
                                    else{
                                        // Enviar através do chord
                                        sprintf(message, "RSP %d %d %d %s %s", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                        
                                        if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                            sprintf(message, "RSP %d %d %d %s %s\n", find_k, seq_n, tempNODE.key, tempNODE.IP, tempNODE.port);
                                            tcpWrite(sucessor.fd, message);
                                        }
                                    
                                    }
                                }                               
                                
                            break; // case RSP
                            
                            case EFND:                      
                                // Enviar ACK
                                getIPPORT(extNODE.IP, extNODE.port, &addr);
                                udpBasicSend(udp_fd, extNODE.IP, extNODE.port,  "ACK");                                
                                
                                // Caso a chave pertença ao nó atual
                                if(belongs(self.key, sucessor.node.key, find_k)){
                                    // Enviar mensagem de resposta ao nó exterior (<EPRED pred pred.IP pred.port\n>)
                                    sprintf(message, "EPRED %d %s %s", self.key, self.IP, self.port);
                                    
                                    if(udpSend(udp_fd, extNODE.IP, extNODE.port, message)==0){ 
                                    }
                                    else{
                                    }
                                    
                                    
                                }
                                
                                
                                else{
                                    // Determinar o primeiro número de sequência disponível
                                    seq_n = getSeqN(WaitList);
                                    WaitList[seq_n].fd = udp_fd; //A procura foi iniciada pelo newfd
                                    WaitList[seq_n].key = find_k;
                                    strcpy(WaitList[seq_n].IP, extNODE.IP);
                                    strcpy(WaitList[seq_n].port, extNODE.port);
                                    
                                
                                    // send FND normal
                                    
                                    // Determinar se o pedido deve ser enviado através do chord ou do sucessor
                                    
                                    if((chord.key == -1) || (chord_occupied) || (distDecision(self.key, sucessor.node.key, chord.key , find_k) == 0) ){
                                            
                                        // Enviar pelo sucessor
                                        sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, self.key, self.IP, self.port);
                                        tcpWrite(sucessor.fd, message);

                                    }
                                    // Caso contrário a mensagem deve seguir via chord
                                    else{                                    
                                        sprintf(message, "FND %d %d %d %s %s", find_k, seq_n, self.key, self.IP, self.port);
                                        // Enviar pelo chord
                                        if(udpSend(udp_fd, chord.IP, chord.port, message)==0){ //A corda não respondeu(após 5 tentativas) logo enviou se o pedido por tcp
                                            sprintf(message, "FND %d %d %d %s %s\n", find_k, seq_n, self.key, self.IP, self.port);
                                            
                                            tcpWrite(sucessor.fd, message);
                                        }
                                    }
                                
                                }

                            break; // case EFND
                            
                            default:
                            break;

                        }  // Switch messageType 
                        
                    }
                    
                break;

                default:
                break;

            } //switch(currSTATE){
        }//for(;counter;--counter){

    }//while(1){
    
    exit(0);  
    
}//int main


// Retorna o nó caso os argumentos tenham sido inseridos corretamente
// Retorna NULL caso contrário
NODE processArguments(int argc, char* argv[]){
    NODE tempNODE; initializeNODE(&tempNODE);
    int key;
    // Nr incorreto de argumentos
    if(argc != 4){
        fprintf(stderr, "WARNING: Invalid number of arguments\n");
        fprintf(stderr, "Correct format: ring i i.IP i.port\n");
        return tempNODE;
    } 
    key = atoi(argv[1]);
    // setNODE(NODE* node, char IP_[], char port_[], int key_)
    setNODE(&tempNODE, argv[2], argv[3], key);

    // Verificar se o nó é válido
    if(isValidNODE(tempNODE)!=1){
        clearNODE(&tempNODE);
        return tempNODE;
    }
    else{
        return tempNODE;
    }
}

void printInterface(){
    fprintf(stdout,"•\tnew\nCriação de um anel contendo apenas o nó.\n\n");
    fprintf(stdout,"•\tbentry boot boot.IP boot.port\nEntrada do nó no anel ao qual pertence o nó boot com endereço IP boot.IP e porto boot.port. Embora apareça primeiro nesta lista, este será o último comando a ser implementado\n\n");
    fprintf(stdout,"•\tpentry pred pred.IP pred.port\nEntrada do nó no anel sabendo que o seu predecessor será o nó pred com endereço IP pred.IP e porto pred.port. Com este comando, um nó pode entrar no anel sem que a funcionalidade de procura de chaves esteja ativa.\n\n");
    fprintf(stdout,"•\tchord i i.IP i.port\nCriação de um atalho para o nó i com endereço IP i.IP e porto i.port.\n\n");
    fprintf(stdout,"•\techord\nEliminação do atalho.\n\n");
    fprintf(stdout,"•\tshow\nMostra o estado do nó, consistindo em: (i) a sua chave, endereço IP e porto; (ii) a chave, endereço IP e porto do seu sucessor; (iii) a chave, endereço IP e porto do seu predecessor; e, por fim, (iv) a chave, endereço IP e porto do seu atalho, se houver\n\n");
    fprintf(stdout,"•\tleave\nSaída do nó do anel.\n\n");
    fprintf(stdout,"•\texit\nFecho da aplicação.\n\n");
    return;
}

/*
show  Mostra o estado do nó, consistindo em: (i) a sua chave, endereço IP e porto; (ii) a
chave, endereço IP e porto do seu sucessor; (iii) a chave, endereço IP e porto do
seu predecessor; e, por fim, (iv) a chave, endereço IP e porto do seu atalho, se
houver.
*/

void showConnections(NODE self, NODE sucessor, NODE predecessor, NODE atalho){  
    printf("Self: %d %s %s\n", self.key        , self.IP        , self.port);
    if(sucessor.key!=-1)
        printf("Suce: %d %s %s\n", sucessor.key    , sucessor.IP    , sucessor.port);
    if(predecessor.key!=-1)
        printf("Pred: %d %s %s\n", predecessor.key , predecessor.IP , predecessor.port);
    if(atalho.key!=-1)
        printf("Atalh: %d %s %s\n", atalho.key , atalho.IP , atalho.port);
    //TODO Verificar se esta conforme enunciado
    return;
}



// Determina o primeiro número de sequência disponível
// Caso exista um número de sequência disponível retorna esse número (entre 0 e 99)
// Caso todos os números de sequência estejam ocupados retorna -1
int getSeqN(WaitList WaitList[]){
    int seq_n = -1; // Número de sequência
    for(int i = 0; i < 100; i++){
        if(WaitList[i].key == -1)
            return i; // Retorna o primeiro número de sequência disponível
    }
    return seq_n;
}

void negativeOnes (WaitList *WaitList,int length){
  int i;
    for(i = 0 ; i < length ; i++){
        WaitList[i].key = -1;
        WaitList[i].fd = -1;
    }
    return;
}

void leaveFunction(CONNECTION *sucessor, CONNECTION *predecessor, NODE *chord, NODE *extNODE, WaitList WaitList[]){
    // Close File descriptors
    close((*sucessor).fd);
    close((*predecessor).fd);

    // Clear connections
    initializeCONNECTION(&(*sucessor));
    initializeCONNECTION(&(*predecessor));
    
    // Clear nodes
    clearNODE(&(*chord));
    clearNODE(&(*extNODE));

    // Clear waitlist    
    negativeOnes(WaitList, 100);

    return;
}


// Retorna 1 caso seja para enviar pelo chord
// Retorna 0 caso seja para enviar pelo sucessor
int distdecision(int chord, int sucessor, int key){ 
    // em 𝑎(𝑗), se 𝑎(𝑗) existir e a distância de 𝑎(𝑗) a 𝑘 for menor do que a distância de 𝑠(𝑗) a 𝑘, 
        //𝑑𝑁(𝑎(𝑗), 𝑘) < 𝑑𝑁(𝑠(𝑗), 𝑘);
    // em 𝑠(𝑗), no caso contrário.

    // No1 -> chord
    // No2 -> sucessor
    if(dist(chord,key) < dist(sucessor,key))
        return 1;
    else
        return 0;
}



//https://stackoverflow.com/questions/11720656/modulo-operation-with-negative-numbers
int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}


//
int dist(int a, int b){
    return mod((b-a),32);
}


// Retorna -1 se pertencer ao próprio nó
// Retorna 0 se for para enviar pelo sucessor
// Retorna 1 se for para enviar pelo chord
int distDecision(int self, int sucessor, int chord, int key){
/*
Considere um nó 𝑗 encarregado de pesquisar 𝑘 por iniciativa do nó 𝑖.
1. Se 𝑑𝑁(𝑗, 𝑘) < 𝑑𝑁(𝑠(𝑗), 𝑘), então 𝑘 pertence a 𝑗 e a pesquisa termina.
2. Pelo contrário, se 𝑑𝑁(𝑗, 𝑘) > 𝑑𝑁(𝑠(𝑗), 𝑘), então 𝑗 delega a pesquisa:
    2.1. em 𝑎(𝑗), se 𝑎(𝑗) existir e 𝑑𝑁(𝑎(𝑗), 𝑘) < 𝑑𝑁(𝑠(𝑗), 𝑘);
    2.2. em 𝑠(𝑗), no caso contrário.
*/

// dist(A,B) ((B-A)%32)

    int djk=0, dsk=0, dak=0;
   
    djk = dist(self,     key); 
    dsk = dist(sucessor, key); 
    dak = dist(chord,    key); 
    
    if(djk<0)
        djk = djk*(-1);
    if(dsk<0)
        dsk = dsk*(-1);
    if(dak<0)
        dak = dak*(-1);
    
    
    if(djk < dsk){ // 𝑑𝑁(𝑗, 𝑘) < 𝑑𝑁(𝑠(𝑗), 𝑘)
        // key pertence a self e a pesquisa termina.
        return -1;
    }
    
    if( (chord!=-1) && (dak < dsk)){ // 𝑑𝑁(𝑎(𝑗), 𝑘) < 𝑑𝑁(𝑠(𝑗), 𝑘)
        // delega a pesquisa no chord
        return 1;
    }
    else {
        // delega a pesquisa no sucessor
        return 0;
    }
    
    



}






