// nodes.h

// Este ficheiro consiste na definição do tipo NODE
#ifndef HEADER_NODES
#define HEADER_NODES



// Cada nó é composto por um IP, um porto e uma chave
// Definimos então o tipo NODE como:
typedef struct node{
   char IP[13];
   char port[7];
   int key;
} NODE;

typedef struct connection{
    char command[128]; //Onde vai ser guardada a mensagem até o commando estar completo
    int check; //Guarda o tamanho atual do command
    char* ptr;
    NODE node;
    int fd;
} CONNECTION;

int initializeNODE(NODE* node);
int clearNODE(NODE* node);
int setNODE(NODE* node, char IP_[], char port_[], int key_);
int copyNODE(NODE *destination, NODE original);
int compareNODE(NODE a, NODE b);
void printNODE(NODE node);
int isValidNODE(NODE node);
int isNODEdefined(NODE node);
int initializeCONNECTION(CONNECTION *connection);
int belongs (int self_key, int sucessor_key, int find_key);
int between (int a, int b, int k);

#endif
