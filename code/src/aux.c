
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
#include "../include/aux.h"
#include "../include/link_layer.h"
#include "../include/alarm.h"

int read_frame_header(char serialPort[50], int control_byte[2])
{
    unsigned char flags[5];
    unsigned char buf[BUFFERSIZE];
    int counter = 0;
    bool not_read = true;
    int index = 0;

    while (not_read && alarmEnabled==FALSE)
    {
        int bytes = read(serialPort, buf, 1);
        printf("%d\n", buf[0]);

        if (buf[0] == F && counter == 0)
        {
            flags[0] = F;
            counter++;
        }
        else if (buf[0] == A && flags[0] == F && counter == 1)
        {
            flags[1] = A;
            counter++;
        }
        else if ((buf[0] == control_byte[0] || buf[0] == control_byte[1]) && flags[1] == A && counter == 2)
        {
            if (buf[0] == control_byte[1]){
                index = 1;
            }
            flags[2] = control_byte[index];
            counter++;
        }
        else if (buf[0] == control_byte[index]  ^ A && flags[2] == control_byte && counter == 3)
        {
            flags[3] = control_byte[index] ^ A;
            not_read = false;
            read(serialPort, buf, 1);
            printf("%d\n", buf[0]);
            return index;
        }
        else
        {
            memset(flags, 0, 5);
            counter = 0;
        }
    }
}

int closeFile(int fd, struct termios *oldtio)
{
    sleep(1);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
  

int openFile(char serialPort[50])
{
    int fd = open(serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPort);
        exit(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    // TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    return fd;
}

int bcc_2(int arr[], int n)
{
    // Resultant variable
    int xor_arr = 0;

    // Iterating through every element in
    // the array
    for (int i = 0; i < n; i++)
    {

        // Find XOR with the result
        xor_arr = xor_arr ^ arr[i];
    }

    // Return the XOR
    return xor_arr;
}

int createFrame(unsigned char *frame, unsigned char *controlByte, unsigned char *data, unsigned int length)
{
    frame[0] = F;
    frame[1] = A;
    frame[2] = controlByte;
    frame[3] = *controlByte ^ A;

    for (int i = 0; i < length; i++)
    {
        frame[i + 4] = data[i];
    }

    frame[length + 4] = bbc_2(data, length);
    frame[length + 5] = F;

    return 0;
}

int byteStuffing(unsigned char *frame, int length)
{

    // allocates space for auxiliary buffer (length of the packet, plus 6 bytes for the frame header and tail)
    unsigned char bufferAux[length + 6];

    // passes information from the frame to aux
    for (int i = 0; i < length + 6; i++)
    {
        bufferAux[i] = frame[i];
    }

    int currentPos = DATA_BEGIN;
    // parses aux buffer, and fills in correctly the frame buffer
    for (int i = DATA_BEGIN; i < (length + 6); i++)
    {
        if (bufferAux[i] == F && i != (length + 5))
        {
            frame[currentPos] = ESCAPE;
            frame[currentPos + 1] = F_STUFFING;
            currentPos = currentPos + 2;
        }
        else if (bufferAux[i] == ESCAPE && i != (length + 5))
        {
            frame[currentPos] = ESCAPE;
            frame[currentPos + 1] = ESCAPE_STUFFING;
            currentPos = currentPos + 2;
        }
        else
        {
            frame[currentPos] = bufferAux[i];
            currentPos++;
        }
    }

    return currentPos;
}

int byteDestuffing(unsigned char *frame, int length)
{

    // allocates space for the maximum possible frame length read (length of the data packet + bcc2, already with stuffing, plus the other 5 bytes in the frame)
    unsigned char aux[length + 5];

    // copies the content of the frame (with stuffing) to the aux frame
    for (int i = 0; i < (length + 5); i++)
    {
        aux[i] = frame[i];
    }

    int currentPos = DATA_BEGIN;

    // iterates through the aux buffer, and fills the frame buffer with destuffed content
    for (int i = DATA_BEGIN; i < (length + 5); i++)
    {

        if (aux[i] == ESCAPE)
        {
            if (aux[i + 1] == ESCAPE_STUFFING)
            {
                frame[currentPos] = ESCAPE;
            }
            else if (aux[i + 1] == F_STUFFING)
            {
                frame[currentPos] = F;
            }
            i++;
            currentPos++;
        }
        else
        {
            frame[currentPos] = aux[i];
            currentPos++;
        }
    }

    return currentPos;
}

int getOctectsNumber(int l1, int l2){
    return 256*l2 +l1;
}

void getOctets (int fileSize, int *l1, int* l2){
    *l1 = fileSize % 256;
    *l2 = fileSize / 256;
}

int getFileSize(FILE *fp)
{

    int lsize;

    fseek(fp, 0, SEEK_END);
    lsize = (int)ftell(fp);
    rewind(fp);

    return lsize;
}
