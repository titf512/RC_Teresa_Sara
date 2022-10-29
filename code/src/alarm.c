
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "alarm.h"

#define FALSE 0
#define TRUE 1


// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


