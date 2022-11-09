#ifndef _MACROS_H_
#define _MACROS_H_
#include <termio.h>

#define BAUDRATE B9600 
#define NUM_RETR 3
#define TIMEOUT 3
#define FALSE 0
#define TRUE 1
#define BUFFERSIZE 256

#define TRANSMITTER 1
#define RECEIVER 0

#define SUPERVISION 0
#define INFORMATION 1
#define DATA_BEGIN 4

// ---- macros for data link layer ----
#define MAX_DATA_SIZE 1024                             // max size of a data packet
#define MAX_PACK_SIZE (MAX_DATA_SIZE + 4)              // max size of a data packet and head
#define MAX_SIZE (MAX_PACK_SIZE + 6)                   // max size of data in a frame, head, header and tail
#define MAX_SIZE_FRAME (((MAX_PACK_SIZE + 1) * 2) + 5) // max size of a frame with byte stuffing 
                                                       // (MAX_PACK_SIZE + 1)=1029 -> all bytes that can suffer byte stuffing -the packet and the BCC2                                                // 5 -> the bytes that won't (for sure) suffer byte stuffing (flags, bcc1, address and control bytes)


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
#define A_T 0x03  // A for commands sent by transmitter and answers sent by receiver
#define A_R 0x01  // A for commands sent by receiver and anwers sent by transmitter

#define ESCAPE_STUFFING 0x5D
#define F_STUFFING 0x5E
#define ESCAPE 0x7D

// ---- macros for application layer ----

#define C_DATA 0x01
#define C_START 0x02
#define C_END 0x03

#define T_FILESIZE 0x00
#define T_FILENAME 0x01

#endif
