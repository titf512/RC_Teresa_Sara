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

#define BUF_SIZE 256

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

    // Open serial port device for reading and writing, and not as controlling tty
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

    printf("New termios structure set\n");

    // Create string to send

    unsigned char buf[BUF_SIZE] = {0};
    unsigned char codes[100] = {F, A, C_SET, BCC, F};
    unsigned char answer[100];
    bool stop = true;
    int data[100] = {0x00, 0x01};
    int c = 0;

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    while (alarmCount < 3 && alarmEnabled == FALSE)
    {
        int bytes = write(fd, codes, 5);
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

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    while (alarmCount < 3 && alarmEnabled == FALSE)
    {
        unsigned char dataFlags[100] = {F, A, C_0, A ^ C_0};
        int bytes = write(fd, dataFlags, 4);
        write(fd, data, 5);
        dataFlags[0] = bcc_2(data, 5);
        dataFlags[1] = F;
        write(fd, dataFlags, 2);
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
                else if ((answer[0] == RR_0 ^ A) && flags[2] == RR_0 && counter == 3 && c == 0)
                {
                    flags[3] = RR_0 ^ A;
                    not_read = false;
                    int bytes = read(fd, answer, 1);
                    printf("%d\n", answer[0]);
                    break;
                }
                else if ((answer[0] == RR_1 ^ A) && flags[2] == RR_1 && counter == 3 && c == 1)
                {
                    flags[3] = RR_1 ^ A;
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