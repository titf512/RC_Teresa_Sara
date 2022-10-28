#ifndef _AUX_H_
#define _AUX_H_

#include "macros.h"
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

struct termios *oldtio;
struct termios newtio;

int read_frame_header(char serialPort[50], int control_byte[2]);

int closeFile(int fd, struct termios *oldtio);

int openFile(char serialPort[50]);

int bcc_2(int arr[], int n);

void getOctets (int fileSize, int *l1, int* l2);

int getOctectsNumber(int l1, int l2);

long int findSize(int fp);
#endif // _AUX_H_