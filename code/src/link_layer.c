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
#include "macros.h"
#include "aux.h"
#include "link_layer.h"


volatile int STOP = FALSE;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

LinkLayer linkLayer;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{
    linkLayer.baudRate = connectionParameters.baudRate;
    linkLayer.nRetransmissions = connectionParameters.nRetransmissions;
    linkLayer.sequenceNumber = connectionParameters.sequenceNumber;
    strcpy(linkLayer.serialPort, connectionParameters.serialPort);
    linkLayer.timeout = connectionParameters.timeout;
    linkLayer.role = connectionParameters.role;
    strcpy(linkLayer.frame, connectionParameters.frame);
    linkLayer.frameSize = connectionParameters.frameSize;

    int ret = -1;
    int fd;

    if ((fd = openNonCanonical(linkLayer.serialPort)) == -1)
    {
        return -1;
    }
    alarmEnabled = TRUE;
    if (connectionParameters.role == TRANSMITTER)
    {

        unsigned char codes[6] = {F, A, C_SET, A ^ C_SET, F};

        // Set alarm function handler
        (void)signal(SIGALRM, alarmHandler);
      
        while (alarmCount < 3)
        {
            if (write(fd, codes, 5) < 0)
            {
                return -1;
            }
    
            int control_byte[2] = {C_UA, 0};
            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

            ret = read_frame_header(fd, control_byte);

            if(alarmEnabled==FALSE){
                alarmEnabled = TRUE;
            }else{
                break;
            }
           
        }

        if (ret == -1)
        {
            return -1;
        }
        else
            return fd;
    }

    else if (connectionParameters.role == RECEIVER)
    {
        
        int control_byte[2] = {C_SET, 0};
        if (read_frame_header(fd, control_byte) == 0)
        {
            unsigned char codes[6] = {F, A, C_UA, A ^ C_UA, F};
            if (write(fd, codes, 5) < 0)
            {
                return -1;
            }
            else
                return fd;
           
        }
        else
        {
            return -1;
        }
    }else
    {
        return -1;
    }
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, char *buf, int bufSize)
{

    unsigned char controlByte;
    bool dataSent = FALSE;
    int wantedBytes[2];

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
    int controlByteReceived = 0;

    linkLayer.frameSize = byteStuffing(linkLayer.frame, bufSize);

    while (!dataSent)
    {
        while (alarmCount < linkLayer.nRetransmissions)
        {
            if (write(fd, linkLayer.frame, linkLayer.frameSize + 6) == -1)
            {
                return -1;
            }

            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

            controlByteReceived = read_frame_header(fd, wantedBytes);

            if (controlByteReceived >= 0)
            { // read_value é o índice do wantedByte que foi encontrado
                // Cancels alarm
                alarm(0);
                break;
            }
        }

        if (controlByteReceived == 0)
            dataSent = TRUE;
    }

    if (linkLayer.sequenceNumber == 0)
        linkLayer.sequenceNumber = 1;
    else if (linkLayer.sequenceNumber == 1)
        linkLayer.sequenceNumber = 0;
    else
        return -1;

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int sequenceNr = 0;

int llread(unsigned char *packet, int fd)
{
    int frameSize;
    int wantedBytes[2];
    wantedBytes[0] = S_0;
    wantedBytes[1] = S_1;
    bool packetComplete = false;

    while (!packetComplete)
    {
        read_frame_header(fd, wantedBytes);

        frameSize = byteDestuffing(linkLayer.frame, linkLayer.frameSize);

        int controlByte;
        if (linkLayer.frame[2] == S_0)
            controlByte = 0;
        else if (linkLayer.frame[2] == S_1)
            controlByte = 1;

        unsigned char responseByte;
        // if bcc_2 is correct
        if (linkLayer.frame[frameSize - 2] == bcc_2(&linkLayer.frame[DATA_BEGIN], frameSize - 6))
        {
            // frame is duplicated
            if (controlByte != linkLayer.sequenceNumber)
            {
                // sends confirmation and ignores data
                if (controlByte == 0)
                {
                    responseByte = RR_1;
                    linkLayer.sequenceNumber = 1;
                }
                else
                {
                    responseByte = RR_0;
                    linkLayer.sequenceNumber = 0;
                }
            }
            // received new frame
            else
            {
                // saves data
                for (int i = 0; i < frameSize - 6; i++)
                {
                    packet[i] = linkLayer.frame[DATA_BEGIN + i];
                }

                packetComplete = true;

                if (controlByte == 0)
                {
                    responseByte = RR_1;
                    linkLayer.sequenceNumber = 1;
                }
                else
                {
                    responseByte = RR_0;
                    linkLayer.sequenceNumber = 0;
                }
            }
        }
        // if bcc2 is not correct
        else
        {
            // frame is duplicated
            if (controlByte != linkLayer.sequenceNumber)
            {
                // sends confirmation and ignores data
                if (controlByte == 0)
                {
                    responseByte = RR_1;
                    linkLayer.sequenceNumber = 1;
                }
                else
                {
                    responseByte = RR_0;
                    linkLayer.sequenceNumber = 0;
                }
            }
            // new frame
            else
            {
                // data is wrong, asks for new packet
                if (controlByte == 0)
                {
                    responseByte = REJ_0;
                    linkLayer.sequenceNumber = 0;
                }
                else
                {
                    responseByte = REJ_1;
                    linkLayer.sequenceNumber = 1;
                }
            }
        }

        createSupervisionFrame(linkLayer.frame, responseByte, RECEIVER);

        // send supervision frame
        if (write(fd, linkLayer.frame, 5) == -1)
        {
            closeNonCanonical(fd, &oldtio);
            return -1;
        }

        printf("Response frame: (%x)\n", linkLayer.frame[2]);
    }

    return (frameSize - 6); // number of bytes of the data packet read
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd)
{
    int wantedBytes[2] = {DISC, 0};

    if (linkLayer.role == LlTx)
    {

        // creates DISC frame
        if (createSupervisionFrame(linkLayer.frame, DISC, TRANSMITTER) != 0)
            return -1;

        printf("Sent DISC frame\n");

        int ret = -1;

        alarmCount = 0;

        while (alarmCount < 3)
        {
            // send DISC frame to receiver
            if (write(fd, linkLayer.frame, 5) == -1)
                return -1;

            alarm(linkLayer.timeout);
            ret = read_frame_header(fd, wantedBytes);

            if (ret >= 0)
            {
                // Cancels alarm
                alarm(0);
                break;
            }
        }

        if (ret == -1)
        {
            closeNonCanonical(fd, &oldtio);
            printf("Closing file descriptor\n");
            return -1;
        }

        printf("Received DISC frame\n");

        // creates UA frame
        if (createSupervisionFrame(linkLayer.frame, C_UA, TRANSMITTER) != 0)
            return -1;

        // send DISC frame to receiver
        if (write(fd, linkLayer.frame, 5) == -1)
            return -1;

        printf("Sent UA frame\n");

        return 0;
    }

    else if (linkLayer.role == LlRx)
    {

        if (read_frame_header(fd, wantedBytes) == -1)
            return -1;

        printf("Received DISC frame\n");

        // creates DISC frame
        createSupervisionFrame(linkLayer.frame, DISC, RECEIVER);

        printf("Sent DISC frame\n");

        int ret = -1;
        alarmCount = 0;
        wantedBytes[0] = C_UA;

        while (alarmCount < 3)
        {
            // send DISC frame to receiver
            if (write(fd, linkLayer.frame, 5) == -1)
                return -1;

            alarm(linkLayer.timeout);

            ret = read_frame_header(fd, wantedBytes);
            alarmEnabled = TRUE;

            if (ret >= 0)
            {
                // Cancels alarm
                alarm(0);
                break;
            }
        }

        if (ret == -1)
        {
            closeNonCanonical(fd, &oldtio);
            printf("Closing file descriptor\n");
            return -1;
        }

        printf("Received UA frame\n");

        return 0;
    }
    if (closeNonCanonical(fd, &oldtio) == -1)
        return -1;

    if (close(fd) != 0)
        return -1;

    return 1;
}
