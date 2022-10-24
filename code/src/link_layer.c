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
#include "../include/alarm.h"

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
        while (alarmCount < 3)
        {
            if (write(connectionParameters.serialPort, codes, 5) < 0)
            {
                closeFile(fd, &oldtio);
                return -1;
            }
            
            int control_byte[2] = {C_UA, 0};
            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s
            ret = read_frame_header(linkLayer.serialPort, control_byte);
            alarmEnabled = TRUE;
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
    int controlByte = 0;

    while (!dataSent)
    {
        while (alarmCount < linkLayer.nRetransmissions)
        {
            if (write(fd, linkLayer.frame, bufSize + 6) == -1)
            { ////mudar terceiro parâmetro para o return do create frame
              ////( que vai ser o length da frame)
                closeFile(fd, &oldtio);
                return -1;
            }

            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

            controlByte = read_frame_header(linkLayer.serialPort, wantedBytes);

            if (controlByte >= 0)
            { // read_value é o índice do wantedByte que foi encontrado
                // Cancels alarm
                alarm(0);
                break;
            }
        }

        if (controlByte == 0)
            dataSent = TRUE;
    }

    if (linkLayer.sequenceNumber == 0)
        linkLayer.sequenceNumber = 1;
    else if (linkLayer.sequenceNumber == 1)
        linkLayer.sequenceNumber = 0;
    else
        return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int sequenceNr = 0;

int llread(unsigned char *packet, int fd)
{

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
