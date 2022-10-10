// comandos.c

// Funções dedicadas ao processamento de comandos inseridos pelo utilizador


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>

#include "nodes.h"

#include "comandos.h"


void processCommand(char commandStr[], COMMAND *userCommand , NODE* commandNODE){
    char delim[] = " ";
    char *ptr; 
    char commandCopy[MAXLINE];
    char format[15];

    initializeNODE(&(*commandNODE));
    
    // Instrução
    strcpy(commandCopy, commandStr);   
    ptr = strtok(commandCopy, delim);
        
    // NEW (n)
    if( (strcmp(ptr, "n\n") == 0) || (strcmp(ptr, "new\n") == 0) ){
        *userCommand = NEW;
        return;
    }
    
    // BENTRY (b boot boot.IP boot.port)
    else if( (strcmp(ptr, "b") == 0) || (strcmp(ptr, "bentry") == 0) ){
        // Processar resto do comando
        sprintf(format, "%s %%d %%s %%s\n", ptr);
        
        if(sscanf(commandStr, format, &((*commandNODE).key), (*commandNODE).IP, (*commandNODE).port) == EOF){
            *userCommand = INVALID;
            return;
        }
        else{
            *userCommand = BENTRY;
            return;
        }

    } 
    
    // PENTRY (p pred pred.IP pred.port)
    else if( (strcmp(ptr, "p") == 0) || (strcmp(ptr, "pentry") == 0) ){

        // Processar resto do comando
        sprintf(format, "%s %%d %%s %%s\n", ptr);
        if(sscanf(commandStr, format, &((*commandNODE).key), (*commandNODE).IP, (*commandNODE).port) == EOF){
            *userCommand = INVALID;
            printf("PENTRY INVALID\n");
            return;
        }
        else{
            *userCommand = PENTRY;
            return;
        }

    } 
    
    // CHORD (c i i.IP i.port)
    else if( (strcmp(ptr, "c") == 0) || (strcmp(ptr, "chord") == 0) ){
        
        // Processar resto do comando
        sprintf(format, "%s %%d %%s %%s\n", ptr);
        
        if(sscanf(commandStr, format, &((*commandNODE).key), (*commandNODE).IP, (*commandNODE).port) == EOF){
            *userCommand = INVALID;
            return;
        }
        else{
            *userCommand = CHORD;
            return;
        }
    
    }
    
    // ECHORD (DCHORD porque o E já é para o exit)
    else if( (strcmp(ptr, "d\n") == 0) || (strcmp(ptr, "dchord\n") == 0)  || (strcmp(ptr, "echord\n") == 0)){
        *userCommand = ECHORD;
        return;
    } 
    
    // FIND (f k)
    else if( (strcmp(ptr, "f") == 0) || (strcmp(ptr, "find") == 0) ) {
        
        // Processar resto do comando
        sprintf(format, "%s %%d", ptr);
        if(sscanf(commandStr, format, &((*commandNODE).key) ) == EOF){
            *userCommand = INVALID;
            return;
        }
        else{
            if( ( (*commandNODE).key < 0) || ( (*commandNODE).key > 32)){
                *userCommand = INVALID;
                printf("Invalid key value\n");
                return;
            }
            else{ 
                *userCommand = FIND;
                return;
            }
        }
    
    }
    
    // SHOW (s)
    else if( (strcmp(ptr, "s\n") == 0) || (strcmp(ptr, "show\n") == 0) ){
        *userCommand = SHOW;
    } 
    
    // LEAVE (l)
    else if( (strcmp(ptr, "l\n") == 0) || (strcmp(ptr, "leave\n") == 0) ){
        *userCommand = LEAVE;
        return;
    } 
        
    // EXIT  (e)  
    else if( (strcmp(ptr, "e\n") == 0) || (strcmp(ptr, "exit\n") == 0) ){
        *userCommand = EXIT;
        return;
    }
          
    // INVALID COMMAND 
    else
        *userCommand = INVALID;

    return;
}



int isCommandComplete(CONNECTION *NoOriginador, char resposta[]){
    char buffer[128]="\0";
    char *ptr1;
    char *ptr3; //Apontador que encontra o \n
    int i, n;

    n=read(NoOriginador->fd,buffer,128); //n numero de caracteres para leitura
    if(n==0){return -1;}
    if((NoOriginador->check+n)>128){fprintf(stderr,"Excedeu a capacidade do buffer!");exit(1);}
    for(ptr1=buffer;n>0;n--,(NoOriginador->check)++,ptr1++,(NoOriginador->ptr)++){*(NoOriginador->ptr)=*ptr1;};// ptr na proxima posição livre
    for(n=0,ptr3=(NoOriginador->command);n<(NoOriginador->check) && (*ptr3!='\n');ptr3++,n++); //n (posição do \n no array)
    ptr3++; //Proxima posição para processar
    if(n==(NoOriginador->check)){
        for(i=0;i<128;i++){
            resposta[i]='\0';
        }
        return 0;
    } //Comando incompleto
    for(ptr1=(NoOriginador->command),i=0;i<n+1+1;ptr1++,i++){resposta[i]=*ptr1;};
    for(;i<128;i++){resposta[i]='\0';} //Limpa o resto da resposta
    (NoOriginador->check)-=n+1+1; //Novo tamanho apos print do command( n + \n + \0 )
    (NoOriginador->ptr)-=n+1+1;
    for(i=0,ptr1=(NoOriginador->command);n+i+1<128;ptr1++,ptr3++,i++){*ptr1=*ptr3;}; //Desloca para a esquerda
    for(;n+i+1<128;i++,ptr3++){*ptr3='\0';}; //Limpa o resto do buffer
    return 1;//(Aqui este comando deve ser mandado para processamento)
}
