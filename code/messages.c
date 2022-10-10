// messages.c
// Funções relacionadas com o processamento de mensagens recebidas

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>

#include "nodes.h"
#include "messages.h"



void processMessage(char messageStr[], MESSAGE *messageType, NODE *messageNODE, int *seq_n, int *find_k){ 
    char delim[] = " ";
    char *ptr; 
    char commandCopy[128];
    
    *find_k = -1; *seq_n = -1; // Para o caso específico do FND e RSP
    
    clearNODE(&(*messageNODE));
    
    // Instrução
    strcpy(commandCopy, messageStr);   
    ptr = strtok(commandCopy, delim);
    
    if(strcmp(ptr, "PRED")==0){
        if(sscanf(messageStr, "PRED %d %s %s\n", &((*messageNODE).key), (*messageNODE).IP, (*messageNODE).port) != EOF){
            *messageType = PRED;
        }
        else{
            *messageType = INV;
        }
        return;
    }
    else if(strcmp(ptr, "FND")==0){
        if (sscanf(messageStr, "FND %d %d %d %s %s\n", &(*find_k), &(*seq_n), &((*messageNODE).key), (*messageNODE).IP, (*messageNODE).port ) != EOF) {
            *messageType = FND;
        }
        else{
            *messageType = INV;
        }
        return;
    }
    
    else if(strcmp(ptr, "RSP")==0){
        if (sscanf(messageStr, "RSP %d %d %d %s %s\n", &(*find_k), &(*seq_n), &((*messageNODE).key), (*messageNODE).IP, (*messageNODE).port ) != EOF) {
            *messageType = RSP;
        }
        else{
            *messageType = INV;
        }
        return;
    }
    
    else if(strcmp(ptr, "SELF")==0){
        if(sscanf(messageStr, "SELF %d %s %s\n", &((*messageNODE).key), (*messageNODE).IP, (*messageNODE).port) != EOF){
            *messageType = SELF;
        }
        else{
            *messageType = INV;
        }
        return;
    }
    
    else if(strcmp(messageStr, "ACK")==0){
        *messageType = ACK;
    }
    
    else if(strcmp(ptr, "EFND")==0){
        if (sscanf(messageStr, "EFND %d", &(*find_k)) != EOF) {
            *messageType = EFND;
        }
        else{
            *messageType = INV;
        }
        return;
    }
    else if(strcmp(ptr, "EPRED")==0){
        if(sscanf(messageStr, "EPRED %d %s %s\n", &((*messageNODE).key), (*messageNODE).IP, (*messageNODE).port) != EOF){
            *messageType = EPRED;
        }
        else{
            *messageType = INV;
        }
        return;
    }
}
