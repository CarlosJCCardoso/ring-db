// states.h

#ifndef HEADER_STATES
#define HEADER_STATES


typedef enum  {BOOT, ALONE, WAIT_EPRED, WAIT_SELF, WAIT_SUCESSOR, WAIT_CHORD_ACK , CONNECTED} STATE; 

void changeState(STATE* prevSTATE, STATE* currSTATE, STATE nextSTATE);

#endif
