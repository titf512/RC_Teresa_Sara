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
#include "aux.h"

int read_frame_header(char serialPort[50],int control_byte)
{
    unsigned char flags[5];
    unsigned char buf[BUFFERSIZE];
    int counter = 0;
    bool not_read = true;

    while (not_read)
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
        else if (buf[0] == control_byte && flags[1] == A && counter == 2)
        {
            flags[2] = control_byte;
            counter++;
        }
        else if (buf[0] == control_byte ^ A && flags[2] == control_byte && counter == 3)
        {
            flags[3] = control_byte ^ A;
            not_read = false;
            read(serialPort, buf, 1);
            printf("%d\n", buf[0]);
            return 0;
        }
        else
        {
            memset(flags, 0, 5);
            counter = 0;
        }
    }
}