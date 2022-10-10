// nodes.c
// Este ficheiro consiste na definição do tipo NODE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "enderecos.h"

#include "nodes.h"




// Inicializa os valores do nó como IP="", port="" e key=-1
int initializeNODE(NODE* node){
    if(&(*node) != NULL){
        strcpy( (*node).IP , "");
        strcpy( (*node).port , "");
        (*node).key = -1;
        return 1;
    }
    else{
        return -1;
    }
}

// Limpa os valores do nó para IP="", port="" e key=-1
int clearNODE(NODE* node){
    if(&(*node)  != NULL){
        strcpy( (*node).IP , "");
        strcpy( (*node).port , "");
        (*node).key = -1;
        return 1;
    }
    else{
        return -1;
    }
}

// Define os valores do nó como IP=IP_, port=port_ e key=key_
int setNODE(NODE* node, char IP_[], char port_[], int key_){
    if(&(*node) != NULL){
        strcpy( (*node).IP , IP_);
        strcpy( (*node).port , port_);
        (*node).key = key_;
        return 1;
    }
    else{
        return -1;
    }
}

// Copia os valores de um nó para outro
int copyNODE(NODE *destination, NODE original){
    if( setNODE(&(*destination), original.IP, original.port, original.key) == 1){
        return 1;
    }

    else{
        return -1;
    } 
}


int compareNODE(NODE a, NODE b){
    if( (strcmp(a.IP,b.IP) == 0)  &&  (strcmp(a.port, b.port)==0) && (a.key == b.key) )
       return 1;
    else 
       return 0;
}






// Imprime um nó
void printNODE(NODE node){
    printf("Chave: %d ; IP: %s; Porto: %s\n", node.key, node.IP, node.port);
    return;
}

// Verifica se um nó é válido
// Retorna 1 caso seja válido, retorna -1 caso contrário
int isValidNODE(NODE node){
    // A chave do nó deve estar entre 0 e 32 para ser válida
    if(node.key > 32 || node.key <= 0){
         fprintf(stderr, "WARNING: Incorrect value for key\n");
        return -1;
    }

    // Verificar porto
    if(validate_port(node.port) != 1){
        fprintf(stderr, "WARNING: Invalid Port\n");
        return -1;
    }

    // Verificar IP
    if(validate_ip(node.IP) != 1){ 
        fprintf(stderr, "WARNING: Invalid IP\n");
        return -1;
    }
    return 1;
}


// Verifica se um nó se encontra definido (key =! -1)
int isNODEdefined(NODE node){
    if(node.key != -1)
        return 1;
    else
        return 0;
}


// Inicializa os valores da conexão
int initializeCONNECTION(CONNECTION *connection){
    int i=0;
    if(&(*connection) != NULL){
        initializeNODE( &((*connection).node) );  //Initialize connection node
        (*connection).fd = -1;                    //Initialize rest of connection
        (*connection).check=0;
        (*connection).ptr=(*connection).command;
        for(i=0;i<128;i++)
            (*connection).command[i]='\0';
        return 1;
    }
    else{
        return -1;
    }
}


// Determina se a chave (find_key) pertence ao nó com chave (self_key) que tem como sucessor a chave (sucessor_key)
int belongs (int self_key, int sucessor_key, int find_key){
 
 
    if((self_key == -1) || (sucessor_key == -1) || (find_key==-1))
        return 0;
 
 
    if( find_key == self_key )
        return 1;
        
    if( find_key == sucessor_key)
        return 0;
 
 
    // Caso o nó seja o nó com maior chave (suce < self)
    if( sucessor_key < self_key ){
 
        if(find_key > self_key) // Caso a chave que procuramos seja maior que o maior nó então pertence ao maior nó
            return 1;
        else{                   // Basta verificar que suce > key 
            if(sucessor_key > find_key)
                return 1;
            else
                return 0;
        }
    }
    // Caso contrário
    else{
        // Verificar se suce > key e self <= key   
        if( (sucessor_key > find_key) && (self_key <= find_key)  )
            return 1;
        else
            return 0;
    }  
}


int between (int a, int b, int k){  // a(self) b(sucessor) k(key) // a(origem) b(fim)
    if((a == -1) || (b == -1) || (k==-1))
        return 0;


    if( k == a )
        return 1;
    if( k == b)
        return 0;

    // Caso a > b 
    if( a > b ){
        // Caso a chave que procuramos seja maior que o maior nó então pertence ao maior nó
        if(k > a) 
            return 1;
        else{                   
            if(b > k)
                return 1;
            else
                return 0;
        }
    }

    // Caso contrário
    else{  
        if( (b > k) && (a <= k) )
            return 1;
        else
            return 0;
    } 
} 


