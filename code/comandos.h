// comandos.h

#ifndef HEADER_COMANDOS
#define HEADER_COMANDOS

#define MAXLINE 1024

typedef enum  {NEW, BENTRY, PENTRY, CHORD, ECHORD, SHOW, FIND, LEAVE, EXIT, INVALID} COMMAND; 

void processCommand(char commandStr[], COMMAND *userCommand , NODE *commandNODE);
int isCommandComplete(CONNECTION *NoOriginador, char resposta[]);

#endif



