#ifndef _AUX_H_
#define _AUX_H_

#include "macros.h"

int read_frame_header(char serialPort[50], int control_byte[2]);

int closeFile(int fd, struct termios *oldtio);

int openFile(char serialPort[50]);

int bcc_2(int arr[], int n);

#endif // _AUX_H_