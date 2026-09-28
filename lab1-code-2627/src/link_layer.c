// RCOM 2026/2027
//
// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

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

volatile int alarmEnabled;
volatile int alarmCount;
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d received\n", alarmCount);
}

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
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        return -1;
    }

    alarmEnabled = FALSE;
    alarmCount = 0;


    State state = START;
    unsigned char byte;


    unsigned char ua_frame[5];
    ua_frame[0] = FLAG;
    ua_frame[1] = A_TX;
    ua_frame[2] = C_SET;
    ua_frame[3] = A_TX ^ C_SET;
    ua_frame[4] = FLAG;

    writeBytesSerialPort(ua_frame, 5);
    printf("SET enviado\n");

    while (state != STOP && alarmCount <= llParameters.nRetransmissions) {
        // Read one byte from serial port.
        if (!alarmEnabled){ alarm(llParameters.timeout);}
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
                if (byte == C_UA) state = C_BCC;
                else if (byte == FLAG) state = FLAG_A;
                else state = START;
                break;

            case C_BCC:
                if (byte == (A_TX ^ C_UA)) state = BCC_OK;
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
        alarm(0);
        return -1;
    }

    printf("UA recebida\n");

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
