// udp_sc.h

#ifndef HEADER_UDP_SC
#define HEADER_UDP_SC

#define MAXLINE 1024
int setup_UDP_socket(char IP[], char port[]);
int udpReceive(int fd,  char buffer[], struct sockaddr *addr);
void print_ipv4(struct sockaddr *s);
void getIPPORT(char IP[], char port[], struct sockaddr *s);
int udpSend(int destination_fd, char destination_IP[], char destination_port[], char message[]);
void udpBasicSend(int destination_fd, char destination_IP[], char destination_port[], char message[]);

#endif
