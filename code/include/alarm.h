#ifndef _ALARM_H_
#define _ALARM_H_

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "macros.h"

int alarmEnabled;
int alarmCount;
/**
 * Handles the alarm signal
 * @param signal Signal that is received
 */
void alarmHandler(int signal);

#endif // _ALARM_H_