#ifndef _MACROS_H_
#define _MACROS_H_
#include <termio.h>

#define BAUDRATE B38400 // 38400 is the normal value
#define N_TRIES 3
#define FALSE 0
#define TRUE 1
#define BUFFERSIZE 256

// ---- macros for data link layer ----
#define MAX_DATA_SIZE 1024                             // max size of a data packet
#define MAX_PACK_SIZE (MAX_DATA_SIZE + 4)              // max size of a data packet + 4 bytes for packet head
#define MAX_SIZE (MAX_PACK_SIZE + 6)                   // max size of data in a frame, + 4 bytes fot packet head, + 6 bytes for frame header and tail
#define MAX_SIZE_FRAME (((MAX_PACK_SIZE + 1) * 2) + 5) // max size of a frame, with byte stuffing considered ((1029 * 2) + 5)
                                                       // 1029 -> all bytes that can suffer byte stuffing (and therefore be "duplicated"), that is, the packet and the BCC2
                                                       // 5 -> the bytes that won't (for sure) suffer byte stuffing (flags, bcc1, address and control bytes)

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

#define ESCAPE_STUFFING 0x5D
#define F_STUFFING 0x5E
#define ESCAPE 0x7D

#define _POSIX_SOURCE 1 // POSIX compliant source

// ---- macros for application layer ----

#define C_DATA 0x01
#define C_START 0x02
#define C_END 0x03

#define T_FILESIZE 0x00
#define T_FILENAME 0x01

#endif
