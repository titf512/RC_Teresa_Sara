// Link layer protocol implementation

#include "link_layer.h"
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

#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define BCC_UA A ^ C_UA
#define BCC_SET A ^ C_SET
#define C_0 0x00
#define C_1 0x40
#define RR_0 0x05
#define RR_1 0x85
#define REJ_0 0x01
#define REJ_1 0x81
#define BUF_SIZE 256

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

LinkLayer linkLayer;
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(LinkLayer connectionParameters)
{
    linkLayer = connectionParameters;

    if (connectionParameters.role == LlTx)
    {
        unsigned char codes[100] = {F, A, C_SET, BCC_SET, F};
        unsigned char answer[100];
        bool stop = true;
        int data[100] = {0x00, 0x01};
        int c = 0;

        // Set alarm function handler
        (void)signal(SIGALRM, alarmHandler);

        while (alarmCount < 3 && alarmEnabled == FALSE)
        {
            int bytes = write(connectionParameters.serialPort, codes, 5);
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
                    int bytes = read(connectionParameters.serialPort, answer, 1);
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
                        int bytes = read(connectionParameters.serialPort, answer, 1);
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

    }else if (connectionParameters.role == LlRx){
        unsigned char test[5] = {F, A, C_UA, BCC_UA, F};
        unsigned char codes[100];
        unsigned char flags[5];

        int counter = 0;
        bool not_read = true;

        while (not_read)
        {
            int bytes = read(connectionParameters.serialPort, codes, 1);
            printf("%d\n", codes[0]);

            if (codes[0] == F && counter == 0)
            {
                flags[0] = F;
                counter++;
            }
            else if (codes[0] == A && flags[0] == F && counter == 1)
            {
                flags[1] = A;
                counter++;
            }
            else if (codes[0] == C_SET && flags[1] == A && counter == 2)
            {
                flags[2] = C_SET;
                counter++;
            }
            else if (codes[0] == BCC_SET && flags[2] == C_SET && counter == 3)
            {

                flags[3] = BCC_SET;
                not_read = false;
                printf("%d\n", codes[0]);
                break;
            }
            else
            {
                memset(flags, 0, 5);
                counter = 0;
            }
        }

        write(connectionParameters.serialPort, test, 5);
    }
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize, int c)
{
    (void)signal(SIGALRM, alarmHandler);

    unsigned char dataFlags[4] = {F,A,0,0};
    unsigned char dataFlags_end[2];

    while (alarmCount < linkLayer.nRetransmissions && alarmEnabled == FALSE)
    {
        if (c==0){
            dataFlags[2] = C_0;
            dataFlags[3] = A ^ C_0;
        }else{
            dataFlags[2] = C_1;
            dataFlags[3] = A ^ C_1;
        }

        write(linkLayer.serialPort, dataFlags, 4);
        write(linkLayer.serialPort, buf, bufSize);
        dataFlags_end[0] = bcc_2(buf, bufSize);
        dataFlags_end[1] = F;
        write(linkLayer.serialPort, dataFlags, 2);
        alarmCount++;

        if (alarmEnabled == FALSE)
        {
            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s
            unsigned char flags[5];
            int counter = 0;
            bool not_read = true;
            unsigned char answer[100];

            while (not_read)
            {
                read(linkLayer.serialPort, answer, 1);
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
                else if (answer[0] == REJ_0 && flags[1] == A && counter == 2 && c == 1){
                    flags[2] = REJ_0;
                    counter++;
                }
                else if (answer[0] == REJ_1 && flags[1] == A && counter == 2 && c == 0)
                {
                    flags[2] = REJ_1;
                    counter++;
                }
                else if (answer[0] == RR_0 && flags[1] == A && counter == 2 && c == 1)
                {
                    flags[2] = RR_0;
                    c = 0;
                    counter++;
                }
                else if (answer[0] == RR_1 && flags[1] == A && counter == 2 && c == 0)
                {
                    flags[2] = RR_1;
                    c = 1;
                    counter++;
                }
                else if ((answer[0] == REJ_1 ^ A && flags[1] == REJ_1 && counter == 3 && c == 0)
                    || (answer[0] == REJ_0 ^ A && flags[1] == REJ_0 && counter == 3 && c == 1))
                    {
                        read(linkLayer.serialPort, answer, 1);
                        printf("%d\n", answer[0]);

                        write(linkLayer.serialPort, dataFlags, 4);
                        write(linkLayer.serialPort, buf, bufSize);
                        write(linkLayer.serialPort, dataFlags_end, 2);
                        alarmCount++;
                        break;
                }
                else if ((answer[0] == RR_0 ^ A) && flags[2] == RR_0 && counter == 3 && c == 0)
                {
                    flags[3] = RR_0 ^ A;
                    not_read = false;
                    read(linkLayer.serialPort, answer, 1);
                    printf("%d\n", answer[0]);
                    return bufSize;
                }
                else if ((answer[0] == RR_1 ^ A) && flags[2] == RR_1 && counter == 3 && c == 1)
                {
                    flags[3] = RR_1 ^ A;
                    not_read = false;
                    read(linkLayer.serialPort, answer, 1);
                    printf("%d\n", answer[0]);
                    return bufSize;
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

    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int sequenceNr = 0;

int llread(unsigned char *packet, int c)
{
    unsigned char codes[100];
    unsigned char flags[5];

    int counter = 0;
    bool not_read = true;
    while (not_read)
    {
        int bytes = read(linkLayer.serialPort, codes, 1);
        printf("%d\n", codes[0]);

        if (codes[0] == F && counter == 0)
        {
            flags[0] = F;
            counter++;
        }
        else if (codes[0] == A && flags[0] == F && counter == 1)
        {
            flags[1] = A;
            counter++;
        }
        else if (codes[0] == C_0 && flags[1] == A && counter == 2 && c == 1)
        {
            flags[2] = C_0;
            c = 0;
            counter++;
        }
        else if (codes[0] == C_1 && flags[1] == A && counter == 2 && c == 0)
        {
            flags[2] = C_1;
            c = 1;
            counter++;
        }
        else if ((codes[0] == C_0 ^ A) && flags[2] == C_0 && counter == 3 && c == 0)
        {
            flags[3] = C_0 ^ A;
            counter++;
        }
        else if ((codes[0] == C_1 ^ A) && flags[2] == C_1 && counter == 3 && c == 1)
        {
            flags[3] = C_1 ^ A;
            counter++;
        }
        else if (flags[3] == C_1 ^ A && counter == 4 && c == 1)
        {
            char bcc_2 = codes[0];
            counter++;
            while (true)
            {
                read(fd, codes, 1);

                if (codes[0] == bcc_2)
                {
                    read(fd, codes, 1);
                    if (codes[0] == F)
                        break;
                }

                bcc_2 ^= codes[0];
            }
        }
        else
        {
            memset(flags, 0, 5);
            counter = 0;
        }
    }

    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
