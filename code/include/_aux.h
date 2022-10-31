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

int read_frame_header(int fd, char control_byte[2], char *frame, int mode);

int closeNonCanonical(int fd, struct termios *oldtio);

int openNonCanonical(char serialPort[50]);

char bcc_2(char arr[MAX_DATA_SIZE], int n);

void getOctets(int fileSize, int *l1, int *l2);

int getOctectsNumber(int l1, int l2);

int createFrame(char *frame, int controlByte, char *data, unsigned int length);

int createSupervisionFrame(char *frame, unsigned char controlField, int role);

int byteStuffing(char *frame, int length);

int byteDestuffing( char *frame, int length);

int getFileSize(FILE *fp);

long int findSize(int fp);
#endif // _AUX_H_