// ringv10.h
#ifndef HEADER_RING
#define HEADER_RING

#define max(A,B) ((A)>=(B)?(A):(B))

typedef struct WAIT{
    int key;
    int fd;
    char IP[13];
    char port[7];
} WaitList;


NODE processArguments(int argc, char* argv[]);
void printInterface();
void showConnections(NODE self, NODE sucessor, NODE predecessor, NODE atalho);
int getSeqN(WaitList *WaitList);
void negativeOnes (WaitList *WaitList,int length);
void leaveFunction(CONNECTION *sucessor, CONNECTION *predecessor, NODE *chord, NODE *extNODE, WaitList WaitList[]);
int distdecision(int chord, int sucessor, int key);
int distDecision(int self, int sucessor, int chord, int key);
int mod(int a, int b);
int dist(int a, int b);

#endif
