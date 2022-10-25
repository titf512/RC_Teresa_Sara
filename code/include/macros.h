#ifndef _MACROS_H_
#define _MACROS_H_

#define BAUDRATE B38400 // 38400 is the normal value
#define FALSE 0
#define TRUE 1
#define BUFFERSIZE 256

#define SUPERVISION 0
#define INFORMATION 1
#define DATA_BEGIN 4

#define NUM_RETR 3
#define TIMEOUT 3

#define TRANSMITTER 1
#define RECEIVER 0

#define F 0x7E
#define A 0x03
#define S_0 0x00
#define S_1 0x40
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define RR_0 0X05
#define RR_1 0x85
#define REJ_0 0x01
#define REJ_1 0x81
#define VTIME_VALUE 0
#define VMIN_VALUE 1
#define DISC 0x0B

#define _POSIX_SOURCE 1 // POSIX compliant source

// ---- macros for application layer ----

#define CTRL_DATA 0x01
#define CTRL_START 0x02
#define CTRL_END 0x03

#define TYPE_FILESIZE 0x00
#define TYPE_FILENAME 0x01

#endif
