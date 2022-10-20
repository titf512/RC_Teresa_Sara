// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

// Alarm
#define FALSE 0
#define TRUE 1

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define BCC_UA A ^ C_UA
#define BCC A ^ C_SET
#define C_0 0x00
#define C_1 0x40
#define RR_0 0x05
#define RR_1 0x85

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(int fd, bool flag){
    char source[] = "/dev/tty/S";
    unsigned char codes[100] = {F, A, C_SET, BCC, F};
    unsigned char answer[100];
    bool stop = true;
    int data[100] = {0x00, 0x01};
    int c = 0;

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);
    if(flag){
        while (alarmCount < 3 && alarmEnabled == FALSE)
        {
            int bytes = write("/dev/tty/S" + fd, codes, 5);
            printf("%d bytes written\n", bytes);
            alarmCount++;

            if (alarmEnabled == FALSE)
            {
                alarm(3); // Set alarm to be triggered in 3s
                unsigned char flags[5];
                int counter = 0;
                bool not_read = true;

                while (not_read)
                {
                    int bytes = read(fd, answer, 1);
                    printf("%d\n", answer[0]);

                    if (answer[0] == F && counter == 0)
                    {
                        flags[0] = F;
                        counter++;
                    }
                    else if (answer[0] == A && flags[0] == F && counter == 1)
                    {
                        flags[1] = A;
                        counter++;
                    }
                    else if (answer[0] == C_UA && flags[1] == A && counter == 2)
                    {
                        flags[2] = C_UA;
                        counter++;
                    }
                    else if (answer[0] == BCC_UA && flags[2] == C_UA && counter == 3)
                    {
                        flags[3] = BCC_UA;
                        not_read = false;
                        int bytes = read(fd, answer, 1);
                        printf("%d\n", answer[0]);
                        break;
                    }
                    else
                    {
                        memset(flags, 0, 5);
                        counter = 0;
                    }
                }
                alarmEnabled = TRUE;
            }
        }
    }
}