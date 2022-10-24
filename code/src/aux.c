
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

    struct termios oldtio;
    struct termios newtio;

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