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
#include "_aux.h"
#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

LinkLayer linkLayer;
int sequenceNr = 0;

/**
 * @brief Receiver: reads the opening frame with control SET and if correct replies with the control flag UA
 *        Transmiter: sends the opening frame with control SET and if correct receives confirmation from the receiver with the control flag UA
 *
 * @param connectionParameters
 * @return int
 */
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

    int controlByte = -1;
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

        while (alarmCount < NUM_RETR)
        {
            if (write(fd, codes, 5) < 0)
            {
                return -1;
            }

            char control_byte[2] = {C_UA, 0};
            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

            controlByte = read_frame_header(fd, control_byte, linkLayer.frame, SUPERVISION);

            if (alarmEnabled == FALSE)
            {
                alarmEnabled = TRUE;
            }
            else
            {
                break;
            }
        }

        if (alarmCount == NUM_RETR || controlByte == -1)
        {
            return -1;
        }
        else
            return fd;
    }

    else if (connectionParameters.role == RECEIVER)
    {

        char control_byte[2] = {C_SET, 0};
        if (read_frame_header(fd, control_byte,linkLayer.frame,SUPERVISION) == 0)
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
    }
    else
    {
        return -1;
    }
}

/**
 * @brief Transmitter: builds I-frames with the data packet and sends it to the receiver. 
 *        According to the answering frame from the receiver, it can resend the frame or move on to the next.
 *
 * @param fd
 * @param buf
 * @param bufSize
 * @return int
 */
int llwrite(int fd, char *buf, int bufSize)
{

    unsigned char controlByte;
    bool dataSent = FALSE;
    char wantedBytes[2];

    if (linkLayer.sequenceNumber == 0)
    {
        controlByte = S_0;
    }
    else
    {
        controlByte = S_1;
    }
    alarmCount = 0;
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
            if (write(fd, linkLayer.frame, linkLayer.frameSize) == -1)
            {
                return -1;
            }
   
            alarm(linkLayer.timeout); // Set alarm to be triggered in 3s

            controlByteReceived = read_frame_header(fd, wantedBytes, linkLayer.frame, SUPERVISION);

            if (alarmEnabled == FALSE)
            {
                alarmEnabled = TRUE;
            }
            else
            {
                break;
            }
            if (controlByteReceived >= 0)
            { // read_value é o índice do wantedByte que foi encontrado
                // Cancels alarm
                alarm(0);
                break;
            }
        }
        sleep(1);

        if(alarmCount==NUM_RETR){
            printf("NUMBER OF RETRIES EXCEEDED");
            return -1;
        }

        if (controlByteReceived == 0)
            dataSent = TRUE;
    }

    printf("sequence number %d\n", linkLayer.sequenceNumber);

    if (linkLayer.sequenceNumber == 0){
        linkLayer.sequenceNumber = 1;
        return 0;
    }

    else if (linkLayer.sequenceNumber == 1){
        linkLayer.sequenceNumber = 0;
        return 0;
    }
    else{
        return -1;
    }
    return -1;
}

/**
 * @brief Receiver: reads the I-frame sent from the transmitter. Depending on the frame read it canexecute in four different ways:
 *        - if the BCC2 is correct and the frame is not duplicated, it saves the data and answers with flag RR
 *        - if the BBC2 is correct but the frame is duplicated, it ignores the data and answers with flag RR
 *        - if the BBC2 is incorrect and the frame is not duplicated, it discards the data and answers with the flag REJ
 *        - if the BBC2 is incorrect but the frame is duplicated, it ignores the data and answers with flag RR
 *
 * @param packet
 * @param fd
 * @return int
 */
int llread(unsigned char *packet, int fd)
{
    int frameSize;
    char wantedBytes[2];
    wantedBytes[0] = S_0;
    wantedBytes[1] = S_1;
    bool packetComplete = false;
    unsigned int controlByte;

    while (!packetComplete)
    {
        
        linkLayer.frameSize = read_frame_header(fd, wantedBytes, linkLayer.frame, INFORMATION);
    
        frameSize = byteDestuffing(linkLayer.frame, linkLayer.frameSize-5);

        if (linkLayer.frame[2] == S_0)
        {
            controlByte = 0;
        }

        else if (linkLayer.frame[2] == S_1)
        {
            controlByte = 1;
        }

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

        linkLayer.frame[0] = F;
        linkLayer.frame[1] = A;
        linkLayer.frame[2] = responseByte;
        linkLayer.frame[3] = (A ^ responseByte);
        linkLayer.frame[4] = F;
     
        // sends supervision frame
        if (write(fd, linkLayer.frame, 5) == -1)
        {
            closeNonCanonical(fd, &oldtio);
            return -1;
        }

        printf("Response frame: (%x)\n", linkLayer.frame[2]); // prints the answering C flag
    }

    return (frameSize - 6); // number of bytes of the data packet read
}

/**
 * @brief Transmitter: sends DISC flag, waits to receive an answering DISC from receiver and returns an UA flag.
 *        Receiver: receives DISC flag from transmitter, sends a DISC frame and, finnaly, confirms that it has received an UA flag.
 * 
 * @param fd 
 * @return int 
 */
int llclose(int fd)
{
    char wantedBytes[2];
    wantedBytes[0] = DISC;
    wantedBytes[1] = 0;

    if (linkLayer.role == TRANSMITTER)
    {

        // creates DISC frame
        linkLayer.frame[0] = F;
        linkLayer.frame[1] = A;
        linkLayer.frame[2] = DISC;
        linkLayer.frame[3] = A^DISC;
        linkLayer.frame[4] = F;
        printf("Sent DISC frame\n");

        int ret = -1;

        alarmCount = 0;

        while (alarmCount < NUM_RETR)
        {
           
            // send DISC frame to receiver
            if (write(fd, linkLayer.frame, 5) == -1)
                return -1;
       
            alarm(linkLayer.timeout);
            
            ret = read_frame_header(fd, wantedBytes,linkLayer.frame,SUPERVISION);

            if (alarmEnabled == FALSE)
            {
                alarmEnabled = TRUE;
            }
            else
            {
                break;
            }

            if (ret >= 0)
            {
                // Cancels alarm
                alarm(0);
                break;
            }
        }

        if (alarmCount == NUM_RETR)
        {
            printf("NUMBER OF RETRIES EXCEEDED");
            return -1;
        }

        sleep(1);

        if (ret == -1)
        {
            closeNonCanonical(fd, &oldtio);
            printf("Closing file descriptor\n");
            return -1;
        }

        printf("Received DISC frame\n");
        

        // creates UA frame
        linkLayer.frame[0] = F;
        linkLayer.frame[1] = A;
        linkLayer.frame[2] = C_UA;
        linkLayer.frame[3] = A ^ C_UA;
        linkLayer.frame[4] = F;
        // send UA frame to receiver
        if (write(fd, linkLayer.frame, 5) == -1)
            return -1;
        printf("Sent UA frame\n");


        return 0;
    }

    else if (linkLayer.role == RECEIVER)
    {
        if (read_frame_header(fd, wantedBytes,linkLayer.frame,SUPERVISION) == -1)
            return -1;

        printf("Received DISC frame\n");

        // creates DISC frame
        linkLayer.frame[0] = F;
        linkLayer.frame[1] = A;
        linkLayer.frame[2] = DISC;
        linkLayer.frame[3] = A ^ DISC;
        linkLayer.frame[4] = F;

        printf("Sent DISC frame\n");

        (void)signal(SIGALRM, alarmHandler);

        int ret = -1;
        alarmCount = 0;
        wantedBytes[0] = C_UA;

        while (alarmCount < linkLayer.nRetransmissions)
        {
        
            // send DISC frame to receiver
            if (write(fd, linkLayer.frame, 5) == -1)
                return -1;
    
            alarm(linkLayer.timeout);

           
            ret = read_frame_header(fd, wantedBytes,linkLayer.frame, SUPERVISION);

            if (alarmEnabled == FALSE)
            {
                alarmEnabled = TRUE;
            }
            else
            {
                break;
            }
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
