//tcp_sc.h

#ifndef HEADER_TCP_SC
#define HEADER_TCP_SC

#define MAXLINE 1024
int openTCPserver(char IP[],char port[]);
int TCPconnect(char IP[], char port[]);
void tcpWrite(int destination_fd, char message[]);

#endif
