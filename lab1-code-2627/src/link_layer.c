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
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP
} State;

////////////////////////////////////////////////
// ALARM
////////////////////////////////////////////////

volatile int alarmEnabled;
volatile int alarmCount;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d received\n", alarmCount);
}

static int sendSupervisionFrame (unsigned char a, unsigned char c) {
    unsigned char frame[5] = {FLAG, a, c, a ^ c, FLAG};
    return writeBytesSerialPort(frame, 5);
}

State stateMachine(State state, unsigned char byte, unsigned char a, unsigned char c) {

    switch (state) {
        case START:
            if (byte == FLAG) state = FLAG_RCV;
            break;
            
        case FLAG_RCV:
            if (byte == a) state = A_RCV;
            else if (byte == FLAG) state = FLAG_RCV;
            else state = START;
            break;

        case A_RCV:
            if (byte == c) state = C_RCV;
            else if (byte == FLAG) state = FLAG_RCV;
            else state = START;
            break;

        case C_RCV:
            if (byte == (a ^ c)) state = BCC_OK;
            else if (byte == FLAG) state = FLAG_RCV;
            else state = START;
            break;

        case BCC_OK:
            if (byte == FLAG) state = STOP;
            else state = START;
            break;
            
        case STOP:
            break;
    }

    return START;
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

    while (state != STOP && alarmCount <= llParameters.nRetransmissions) {
        // Read one byte from serial port.
        if (!alarmEnabled){ 
            sendSupervisionFrame(A_TX, C_SET);
            printf("SET enviado (tentativa %d)\n", alarmCount + 1);
            alarm(llParameters.timeout);
            alarmEnabled = TRUE;
        }

        if(readByteSerialPort(&byte) > 0) state = stateMachine(state, byte, A_TX, C_UA);
    }

    alarm(0);

    if (state != STOP) {
        printf("Sem resposta após %d tentativas\n", llParameters.nRetransmissions + 1);
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

    printf("Serial port %s opened\n", llParameters.serialPort);

    State state = START;
    unsigned char byte;

    while (state != STOP) {
        // Read one byte from serial port.
        if (readByteSerialPort(&byte) > 0) {
            printf("byte = 0x%02X\n", byte);
            state = stateMachine(state, byte, A_TX, C_SET);
        }
    }

    printf("SET recebido\n");

    sendSupervisionFrame(A_TX, C_UA);
    printf("UA recebido\n");
    
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
