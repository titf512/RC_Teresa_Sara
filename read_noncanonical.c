// Read from serial port in non-canonical mode
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
#define BCC A ^ C_UA
#define BCC_SET  A ^ C_SET
#define C_0 0x00
#define C_1 0x40
#define RR_0 0x05
#define RR_1 0x85

#define BUF_SIZE 256

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

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

    printf("New termios structure set\n");
   

    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

    unsigned char test[5] = {F, A, C_UA, BCC, F};
    unsigned char codes[100];
    unsigned char flags[5];

    int counter = 0;
    bool not_read = true;

    while (not_read){
        int bytes = read(fd, codes, 1);
        printf("%d\n", codes[0]);

        if (codes[0] == F && counter==0)
        {
            flags[0] = F;
            counter++;
        }
        else if (codes[0] == A && flags[0]==F && counter==1){
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

    int c = 1;
    write(fd, test, 5);

    while (not_read)
    {
        int bytes = read(fd, codes, 1);
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
            while (true){
                read(fd, codes, 1);

                if (codes[0] == bcc_2){
                    read(fd, codes, 1);
                    if (codes[0] == F) break;
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

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}