// messages.h
// Definição do tipo MESSAGE 

typedef enum  {PRED, SELF, FND, RSP, ACK, EFND, EPRED, INV} MESSAGE;

void processMessage(char messageStr[], MESSAGE *messageType, NODE *messageNODE, int *seq_n, int *find_k);
