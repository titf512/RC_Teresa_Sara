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
#include "alarm.h"

struct termios oldtio;
struct termios newtio;

int read_frame_header(int fd, char *control_byte, char *frame, int mode);

int closeNonCanonical(int fd, struct termios *oldtio);

int openNonCanonical(char serialPort[50]);

char bcc_2(char *arr, int n);

void getOctets(int fileSize, int *l1, int *l2);

int getOctectsNumber(int l1, int l2);

int createFrame(char *frame, int controlByte, char *data, unsigned int length);

int byteStuffing(char *frame, int length);

int byteDestuffing( char *frame, int length);

int getFileSize(FILE *fp);

long int findSize(int fp);
#endif // _AUX_H_