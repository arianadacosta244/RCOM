// RCOM 2026/2027
//
// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <stdio.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256

#define FLAG 0x7E
#define A_TX 0x03
#define A_RX 0x01
#define C_SET 0x03
#define C_UA 0x07

typedef enum {
    START,
    FLAG_A,
    A_C,
    C_BCC,
    BCC_OK,
    STOP
} State;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llOpenTx(LinkLayer llParameters)
{
    // ----------------------------------------------------
    // This example code shows how to open the serial port and send a string.
    // TODO: Adapt and extend this code according to the specifications of the project.
    // ----------------------------------------------------

    if (openSerialPort(llParameters.serialPort, llParameters.baudRate) < 0)
    {
        perror("openSerialPort");
        return -1;
    }

    printf("Serial port %s opened\n", llParameters.serialPort);

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    for (int i = 0; i < BUF_SIZE; i++)
    {
        buf[i] = 'a' + i % 26;
    }

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    buf[5] = '\n';

    int bytes = writeBytesSerialPort(buf, BUF_SIZE);
    printf("%d bytes written to serial port\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    // Close serial port
    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        return -1;
    }

    printf("Serial port %s closed\n", llParameters.serialPort);

    return 0;
}

int llOpenRx(LinkLayer llParameters)
{
    // ----------------------------------------------------
    // This example code shows how to open the serial port and receive a string.
    // TODO: Adapt and extend this code according to the specifications of the project.
    // ----------------------------------------------------

    if (openSerialPort(llParameters.serialPort, llParameters.baudRate) < 0)
    {
        return -1;
    }

    State state = START;
    unsigned char byte;

    while (state != STOP) {
        // Read one byte from serial port.
        if (readByteSerialPort(&byte) > 0) {
            switch (state) {
            case START:
                if (byte == FLAG) state = FLAG_A;
                break;
            
            case FLAG_A:
                if (byte == A_TX) state = A_C;
                else if (byte == FLAG) state = FLAG_A;
                else state = START;
                break;

            case A_C:
                if (byte == C_SET) state = C_BCC;
                else if (byte == FLAG) state = FLAG_A;
                else state = START;
                break;

            case C_BCC:
                if (byte == (A_TX ^ C_SET)) state = BCC_OK;
                else if (byte == FLAG) state = FLAG_A;
                else state = START;
                break;

            case BCC_OK:
                if (byte == FLAG) state = STOP;
                else state = START;
                break;
            
            case STOP:
                break;
            }
        }
    }

    unsigned char ua_frame[5];
    ua_frame[0] = FLAG;
    ua_frame[1] = A_TX;
    ua_frame[2] = C_UA;
    ua_frame[3] = A_TX ^ C_UA;
    ua_frame[4] = FLAG;

    writeBytesSerialPort(ua_frame, 5);
    return 0;
    
}

////////////////////////////////////////////////
// LLSEND
////////////////////////////////////////////////
int llSend(const unsigned char *buf, int bufSize)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLRECEIVE
////////////////////////////////////////////////
int llReceive(unsigned char *packet)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llCloseTx()
{
    // TODO: Implement this function

    return 0;
}

int llCloseRx()
{
    // TODO: Implement this function

    return 0;
}
