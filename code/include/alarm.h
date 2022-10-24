#ifndef _ALARM_H_
#define _ALARM_H_

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "macros.h"

int alarmEnabled = FALSE;
int alarmCount = 0;

/**
 * Handles the alarm signal
 * @param signal Signal that is received
 */
void alarmHandler(int signal);

#endif // _ALARM_H_