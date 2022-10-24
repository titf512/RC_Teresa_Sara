// Link layer protocol implementation

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
#include "../include/macros.h"
#include "../include/aux.h"
#include "../include/link_layer.h"

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

LinkLayer linkLayer;

struct termios oldtio;
struct termios newtio;
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

    int ret = -1;
    int fd;

    if (fd = openFile(linkLayer.serialPort) == -1)
    {
        return -1;
    }

    if (connectionParameters.role == LlTx)
    {
        unsigned char codes[6] = {F, A, C_SET, A ^ C_SET, F};

        // Set alarm function handler
        (void)signal(SIGALRM, alarmHandler);
        // ALTERAR
        while (alarmCount < 3 && alarmEnabled == FALSE)
        {
            if (write(connectionParameters.serialPort, codes, 5) < 0)
            {
                closeFile(fd, &oldtio);
                return -1;
            }

            alarmCount++;

            if (alarmEnabled == FALSE)
            {
                alarm(linkLayer.timeout); // Set alarm to be triggered in 3s
                ret = read_frame_header(linkLayer.serialPort, C_UA);
                alarmEnabled = TRUE;
            }
        }
        if (ret == -1)
        {
            closeFile(fd, &oldtio);
            return -1;
        }
    }

    else if (connectionParameters.role == LlRx)
    {
        if (read_frame_header(linkLayer.serialPort, C_SET) == 0)
        {
            unsigned char codes[6] = {F, A, C_UA, A ^ C_UA, F};
            if (write(connectionParameters.serialPort, codes, 5) < 0)
            {
                closeFile(fd, &oldtio);
                return -1;
            }
        }
        else
        {
            closeFile(fd, &oldtio);
            return -1;
        }
    }
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{

    unsigned char response[BUFFERSIZE];
    unsigned char controlByte;
    bool dataSent = FALSE;
    unsigned char wantedBytes[2];
    bool resendFrame = false;

    if (linkLayer.sequenceNumber == 0)
    {
        controlByte = S_0;
    }
    else
    {
        controlByte = S_1;
    }

    createFrame(linkLayer.frame, controlByte, buf, bufSize);

    (void)signal(SIGALRM, alarmHandler);

    if (controlByte == S_0)
    {
        wantedBytes[0] = RR_1;
        wantedBytes[1] = REJ_0;
    }
    else if (controlByte == S_1)
    {
        wantedBytes[0] = RR_0;
        wantedBytes[1] = REJ_1;
    }

    while (alarmCount < linkLayer.nRetransmissions && dataSent == FALSE)
    {
        if (write(fd, linkLayer.frame, bufSize + 6) == -1)
        { ////mudar terceiro parâmetro para o return do create frame
          ////( que vai ser o length da frame)
            closeFile(fd, &oldtio);
            return -1;
        }
        
        alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

        while (alarmEnabled == FALSE)
        {
            int read_value = read_frame_header(linkLayer.serialPort, wantedBytes);

            if (read_value >= 0)
            { // read_value é o índice do wantedByte que foi encontrado
                // Cancels alarm
                alarm(0);
                dataSent = TRUE;
            }
        }
    }

    unsigned char dataFlags[4] = {F, A, 0, 0};
    unsigned char dataFlags_end[2];

    while (alarmCount < linkLayer.nRetransmissions && alarmEnabled == FALSE)
    {
        if (c == 0)
        {
            dataFlags[2] = C_0;
            dataFlags[3] = A ^ C_0;
        }
        else
        {
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
                else if (answer[0] == REJ_0 && flags[1] == A && counter == 2 && c == 1)
                {
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
                else if ((answer[0] == REJ_1 ^ A && flags[1] == REJ_1 && counter == 3 && c == 0) || (answer[0] == REJ_0 ^ A && flags[1] == REJ_0 && counter == 3 && c == 1))
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
