// RCOM 2026/2027
//
// Link layer protocol implementation

#define _POSIX_SOURCE 1

#include "link_layer.h"
#include "serial_port.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 256

#define FLAG 0x7E
#define A_TX 0x03
#define A_RX 0x01
#define C_SET 0x03
#define C_UA 0x07

#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    DATA_RCV,
    BCC_OK,
    STOP
} State;

////////////////////////////////////////////////
// ALARM
////////////////////////////////////////////////

volatile int alarmEnabled;
volatile int alarmCount;

static LinkLayer connectionParams;

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

State stateMachineSupervision(State state, unsigned char byte, unsigned char a, unsigned char c) {

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

        default:
            break;
    }

    return state;
}

State stateMachineInformation(State state, unsigned char byte, unsigned char *c_byte) {
    switch (state) {
    case START:
        if (byte == FLAG) state = FLAG_RCV;
        break;
            
    case FLAG_RCV:
        if (byte == A_TX) state = A_RCV;
        else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;
    
    case A_RCV:
        if (byte == 0x00 || byte == 0x80) {
            *c_byte = byte;
            state = C_RCV;
        } else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;

    case C_RCV:
        if (byte == (A_TX ^ *c_byte)) state = DATA_RCV;
        else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;
    
    default:
        break;
    }
    return state;
}

State stateMachineResponse(State state, unsigned char byte, unsigned char *c_byte) {
    switch (state) {
    case START:
        if (byte == FLAG) state = FLAG_RCV;
        break;

    case FLAG_RCV:
        if (byte == A_TX) state = A_RCV;
        else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;

    case A_RCV:
        if (byte == C_RR0 || byte == C_RR1 || byte == C_REJ0 || byte == C_REJ1) {
            *c_byte = byte;
            state = C_RCV;
        } else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;

    case C_RCV:
        if (byte == (A_TX ^ *c_byte)) state = BCC_OK;
        else if (byte == FLAG) state = FLAG_RCV;
        else state = START;
        break;

    case BCC_OK:
        if (byte == FLAG) state = STOP;
        else state = START;
        break;

    default:
        break;
    }
    return state;
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

    connectionParams = llParameters;

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

        if(readByteSerialPort(&byte) > 0) state = stateMachineSupervision(state, byte, A_TX, C_UA);
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
            state = stateMachineSupervision(state, byte, A_TX, C_SET);
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
    static int ns = 0;

    unsigned char frame[5 + 2 * (MAX_PAYLOAD_SIZE + 1)];
    int idx = 0;

    // Cabeçalho (sem stuffing)
    unsigned char c = (ns == 0) ? 0x00 : 0x80;
    frame[idx++] = FLAG;
    frame[idx++] = A_TX;
    frame[idx++] = c;
    frame[idx++] = A_TX ^ c;

    // BCC2 sobre os dados originais
    unsigned char bcc2 = 0;
    for (int i = 0; i < bufSize; i++) {
        bcc2 ^= buf[i];
    }

    // Dados + BCC2 com byte stuffing
    for (int i = 0; i <= bufSize; i++) {
        unsigned char byte = (i < bufSize) ? buf[i] : bcc2;

        if (byte == FLAG) {
            frame[idx++] = 0x7D;
            frame[idx++] = 0x5E;
        } else if (byte == 0x7D) {
            frame[idx++] = 0x7D;
            frame[idx++] = 0x5D;
        } else frame[idx++] = byte;
    }

    frame[idx++] = FLAG;

    // Enviar e esperar por RR/REJ
    unsigned char expectedRR = (ns == 0) ? C_RR1 : C_RR0;
    unsigned char c_byte = 0;
    unsigned char byte;
    State state = START;

    alarmEnabled = FALSE;
    alarmCount = 0;

    while (alarmCount <= connectionParams.nRetransmissions) {
        if (!alarmEnabled) {
            writeBytesSerialPort(frame, idx);
            printf("Trama I(%d) enviada (tentativa %d)\n", ns, alarmCount + 1);
            alarm(connectionParams.timeout);
            alarmEnabled = TRUE;
        }

        if (readByteSerialPort(&byte) > 0) state = stateMachineResponse(state, byte, &c_byte);

        if (state == STOP) {
            state = START;

            if (c_byte == expectedRR) {
                alarm(0);
                ns = 1 - ns;
                return bufSize;
            }

            if (c_byte == C_REJ0 || c_byte == C_REJ1) {
                printf("REJ recebido, a reenviar\n");
                alarm(0);
                alarmEnabled = FALSE;
            }
        }
    }

    alarm(0);
    printf("llSend falhou após %d tentativas\n", connectionParams.nRetransmissions + 1);
    return -1;
}

////////////////////////////////////////////////
// LLRECEIVE
////////////////////////////////////////////////
int llReceive(unsigned char *packet)
{
    static int expected_ns = 0;
    int valid_frame = 0;
    unsigned char raw_data[(MAX_PAYLOAD_SIZE * 2) + 10]; 
    unsigned char c_byte = 0;

    while (!valid_frame) {
        State state = START;
        int raw_idx = 0;
        unsigned char byte;

        while (state != STOP) {
            if (readByteSerialPort(&byte) > 0) {
                if (state == DATA_RCV) {
                    if (byte == FLAG) state = STOP;
                    else raw_data[raw_idx++] = byte;
                } else {
                    state = stateMachineInformation(state, byte, &c_byte);
                }
            }
        }
        
        int j = 0;

        for (int i = 0; i < raw_idx; i++) {
            if (raw_data[i] == 0x7D) {
                if (raw_data[i+1] == 0x5E) packet[j] = 0x7E;
                else if (raw_data[i+1] == 0x5D) packet[j] = 0x7D;
                i++;

            } else packet[j] = raw_data[i];
            j++;
        }

        unsigned char calc_bcc2 = 0;
        for (int k = 0; k < j-1; k++) {
            calc_bcc2 ^= packet[k];
        }

        if (calc_bcc2 == packet[j-1]) {
            int received_ns = (c_byte == 0x00) ? 0 : 1;

            if (received_ns == expected_ns) {
                if (expected_ns == 0x00) sendSupervisionFrame(A_TX, C_RR1);
                else sendSupervisionFrame(A_TX, C_RR0);

                expected_ns = 1 - expected_ns;
                valid_frame = 1;
                return j- 1;
            } else {
                if (expected_ns == 0x00) sendSupervisionFrame(A_TX, C_RR0);
                else sendSupervisionFrame(A_TX, C_RR1);
            }
        } else {
            if (c_byte == 0x00) sendSupervisionFrame(A_TX, C_REJ0);
            else sendSupervisionFrame(A_TX, C_REJ1);
        }
    }
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
