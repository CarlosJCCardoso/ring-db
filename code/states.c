// states.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "states.h"

void changeState(STATE* prevSTATE, STATE* currSTATE, STATE nextSTATE){
    *prevSTATE = *currSTATE;
    *currSTATE = nextSTATE;
    return;
}

