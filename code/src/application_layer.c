// Application layer protocol implementation
#include <sys/times.h>
#include <sys/time.h>
#include "application_layer.h"
#include "macros.h"
#include "_aux.h"
#include <stdio.h>

AppLayer appLayer;

/**
 * @brief builds the information packet that goes into the I-frame
 * 
 * @param packetBuffer 
 * @param sequenceNumber 
 * @param dataBuffer 
 * @param dataLength 
 * @return int 
 */
int buildDataPacket(char *packetBuffer, int sequenceNumber, unsigned char *dataBuffer, int dataLength)
{

    packetBuffer[0] = C_DATA;

    packetBuffer[1] = (unsigned char)sequenceNumber;

    int l1, l2;
    getOctets(dataLength, &l1, &l2);

    packetBuffer[2] = l2;
    packetBuffer[3] = l1;

    for (int i = 0; i < dataLength; i++)
        packetBuffer[i + 4] = dataBuffer[i];

    return dataLength + 4;
}
/**
 * @brief builds the control packet that goes into I-frame
 * 
 * @param controlByte 
 * @param packetBuffer 
 * @param fileSize 
 * @param fileName 
 * @return int 
 */
int buildControlPacket(unsigned char controlByte, char *packetBuffer, int fileSize, const char *fileName)
{

    packetBuffer[0] = controlByte;

    packetBuffer[1] = T_FILESIZE;

    int length = 0;
    int currentFileSize = fileSize;

    // cicle to separate file size (v1) in bytes
    while (currentFileSize > 0)
    {
        int rest = currentFileSize % 256;
        int div = currentFileSize / 256;
        length++;

        // shifts all bytes to the right, to make space for the new byte
        for (unsigned int i = 2 + length; i > 3; i--)
            packetBuffer[i] = packetBuffer[i - 1];

        packetBuffer[3] = (unsigned char)rest;

        currentFileSize = div;
    }

    packetBuffer[2] = (unsigned char)length;

    packetBuffer[3 + length] = T_FILENAME;

    int fileNameStart = 5 + length; // beginning of v2

    packetBuffer[4 + length] = (unsigned char)(strlen(fileName) + 1); // strlen gets the size fileName string ; add 1 to also include /0 (the end of the string)

    for (unsigned int j = 0; j < (strlen(fileName) + 1); j++)
    {
        packetBuffer[fileNameStart + j] = fileName[j];
    }

    return 3 + length + 2 + strlen(fileName) + 1; // total length of the packet
}
/**
 * @brief reads the control information from the I-frame (either the first or last I-frame)
 * 
 * @param packetBuffer 
 * @param fileSize 
 * @param fileName 
 * @return int 
 */
int parseControlPacket(unsigned char *packetBuffer, int *fileSize, char *fileName)
{

    if (packetBuffer[0] != C_START && packetBuffer[0] != C_END)
    {
        return -1;
    }

    int length1;

    if (packetBuffer[1] == T_FILESIZE)
    {

        *fileSize = 0;
        length1 = (int)packetBuffer[2];

        for (int i = 0; i < length1; i++)
        {
            *fileSize = *fileSize * 256 + (int)packetBuffer[3 + i];
        }
    }
    else
    {
        return -1;
    }

    int length2;
    int fileNameStart = 5 + length1;

    if (packetBuffer[fileNameStart - 2] == T_FILENAME)
    {

        length2 = (int)packetBuffer[fileNameStart - 1];

        for (int i = 0; i < length2; i++)
        {
            fileName[i] = packetBuffer[fileNameStart + i];
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
/**
 * @brief reads the information from the I-frame and, if valid, saves it in the array *data*
 *
 * @param packetBuffer
 * @param data
 * @param sequenceNumber
 * @return int
 */
int parseDataPacket(unsigned char *packetBuffer, unsigned char *data, int *sequenceNumber)
{

    if (packetBuffer[0] != C_DATA)
    {
        return -1;
    }

    *sequenceNumber = (int)packetBuffer[1];

    int dataSize = getOctectsNumber((int)packetBuffer[3], (int)packetBuffer[2]);

    for (int i = 0; i < dataSize; i++)
    {
        data[i] = packetBuffer[i + 4];
    }

    return 0;
}
/**
 * @brief main function that controls all the operations in receiving and sending the data.
 * 
 * @param serialPort 
 * @param role 
 * @param baudRate 
 * @param nTries 
 * @param timeout 
 * @param filename 
 */
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    char frame[BUFFERSIZE];
    LinkLayer connectionParameters;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.sequenceNumber = 0;
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.timeout = timeout;
    strcpy(connectionParameters.frame, frame);
    connectionParameters.frameSize = 2058;

 
    if (strcmp(role, "tx")==0)
    {
        connectionParameters.role = TRANSMITTER;
    }
    else if (strcmp(role, "rx")==0)
    {
        connectionParameters.role = RECEIVER;
    }

    if (connectionParameters.role ==  TRANSMITTER)
    {

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            exit(-1);
            printf("Cannot find the file to transmit\n");
        }

        // fills appLayer fields
        appLayer.status = TRANSMITTER;

        if ((appLayer.fileDescriptor = llopen(connectionParameters)) <= 0)
        {
            exit(-1);
        }

        printf("\n---------------llopen done---------------\n\n");

        char packetBuffer[MAX_PACK_SIZE];
        int fileSize = getFileSize(fp);

        int packetSize = buildControlPacket(C_START, packetBuffer, fileSize, filename);
        
        // sends control start packet, to indicate the start of the file transfer
        if (llwrite(appLayer.fileDescriptor, packetBuffer, packetSize) < 0)
        {
            fclose(fp);
            exit(-1);
        }

        printf("\n---------------STARTING TO SEND FILE---------------\n\n");

        unsigned char data[2058];
        int length_read;
        int sequenceNumber = 0;

        while (1)
        {
            
            // reads a data chunk from the file
            length_read = fread(data, sizeof(unsigned char), MAX_DATA_SIZE, fp);
     
            if (length_read != MAX_DATA_SIZE)
            {
                if (feof(fp))
                {

                    packetSize = buildDataPacket(packetBuffer, sequenceNumber, data, length_read);
                    sequenceNumber = (sequenceNumber + 1) % 256;

                    // sends the last data frame
                    if (llwrite(appLayer.fileDescriptor, packetBuffer, packetSize) < 0)
                    {
                        fclose(fp);
                        exit(-1);
                    }
                    break;
                }
                else
                {
                    perror("error reading file data");
                    exit(-1);
                }
            }

            packetSize = buildDataPacket(packetBuffer, sequenceNumber, data, length_read);
            sequenceNumber = (sequenceNumber + 1) % 256;

            // sends a data frame
            if (llwrite(appLayer.fileDescriptor, packetBuffer, packetSize) < 0)
            {
                fclose(fp);
                exit(-1);
            }
        }

        packetSize = buildControlPacket(C_END, packetBuffer, fileSize, filename);

        // sends control end packet; indicating the end of the file transfer
        if (llwrite(appLayer.fileDescriptor, packetBuffer, packetSize) < 0)
        {
            fclose(fp);
            exit(-1);
        }

        printf("\n---------------ENDED SENDING FILE---------------\n\n");

        if (llclose(appLayer.fileDescriptor) < 0)
            exit(-1);

        printf("\n---------------llclose done---------------\n\n");

        if (fclose(fp) != 0)
            exit(-1);

        exit(0);
    }

    else if (connectionParameters.role == RECEIVER)
    {

        struct timeval start, end;
        int bitsReceived = 0;
        // fills appLayer fields
        appLayer.status = RECEIVER;


        if ((appLayer.fileDescriptor = llopen(connectionParameters)) <= 0)
        {
            exit(-1);
        }

        printf("\n---------------llopen done---------------\n");
        

        unsigned char packetBuffer[MAX_PACK_SIZE];

        int packetSize;
        int fileSize;
        unsigned char data[MAX_DATA_SIZE];
        char fileName[255];

        /* Mark beginning time */
        gettimeofday(&start, NULL);

         usleep(250000);

        packetSize = llread(packetBuffer, appLayer.fileDescriptor);
        
        if (packetSize < 0)
        {
            exit(-1);
        }

        bitsReceived += packetSize * 8;

        // if start control packet was received
        if (packetBuffer[0] == C_START)
        {

            if (parseControlPacket(packetBuffer, &fileSize, fileName) < 0)
            {
                exit(-1);
            }
        }
        else
        {
            exit(-1);
        }

        FILE *fp = fopen("penguin-received.gif", "w");
        if (fp == NULL)
            exit(-1);

        int expectedSequenceNumber = 0;

        // starts received data packets (file data)
        while (1)
        {
            packetSize = llread(packetBuffer, appLayer.fileDescriptor);
            if (packetSize < 0)
            {
                exit(-1);
            }

            bitsReceived += packetSize * 8;

            // received data packet
            if (packetBuffer[0] == C_DATA)
            {
                int sequenceNumber;

                if (parseDataPacket(packetBuffer, data, &sequenceNumber) < 0)
                    exit(-1);

                // if sequence number doesn't match
                if (expectedSequenceNumber != sequenceNumber)
                {
                    printf("Sequence number does not match!\n");
                    exit(-1);
                }

                expectedSequenceNumber = (expectedSequenceNumber + 1) % 256;

                int dataLength = packetSize - 4;

                // writes to the file the content read from the serial port
                if (fwrite(data, sizeof(unsigned char), dataLength, fp) != dataLength)
                {
                    exit(-1);
                }
            }
            // received end packet; file was fully transmitted
            else if (packetBuffer[0] == C_END)
            {
                break;
            }
        }

        gettimeofday(&end, NULL);
        double timeSpent = (end.tv_sec - start.tv_sec) * 1e6;
        timeSpent = (timeSpent + (end.tv_usec - start.tv_usec)) * 1e-6;

        printf("bitsReceived =  %d\n", bitsReceived);
        printf("timeSpent =  %lf\n", timeSpent);
        double R = bitsReceived / timeSpent;
        double baudRate = 9600.0;
        double S = R / baudRate;

        printf("\nEstatitica da eficiencia S = %lf\n\n", S);

        if (getFileSize(fp) != fileSize)
        {
            printf("file size does not match\n");
            exit(-1);
        }

        int fileSizeEnd;
        char fileNameEnd[255];

        if (parseControlPacket(packetBuffer, &fileSizeEnd, fileNameEnd) < 0)
        {
            exit(-1);
        }

        if ((fileSize != fileSizeEnd) || (strcmp(fileNameEnd, fileName) != 0))
        {
            printf("Information in start and end packets does not match");
            exit(-1);
        }

        // close, in non canonical
        if (llclose(appLayer.fileDescriptor) < 0)
            exit(-1);

        printf("\n---------------llclose done---------------\n\n");

        exit(0);
    }
}
