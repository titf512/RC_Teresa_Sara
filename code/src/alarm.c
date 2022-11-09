
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "alarm.h"

#define FALSE 0
#define TRUE 1



/**
 * @brief Alarm function handler
 *
 * @param signal
 */
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


