// RCOM 2026/2027
//
// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

#define PACKET_START 1
#define PACKET_DATA 2
#define PACKET_END 3

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // ----------------------------------------------------
    // TODO: Adapt and extend this code according to the specifications of the project.
    // ----------------------------------------------------

    LinkLayer llParameters = {
        .baudRate = baudRate,
        .nRetransmissions = nTries,
        .timeout = timeout,
    };
    strcpy(llParameters.serialPort, serialPort);

    if (strcmp(role, "tx") == 0)
    {
        if (llOpenTx(llParameters) < 0) {
            printf ("Erro ao abrir a ligação\n");
            return;
        }
        FILE *file = fopen(filename, "rb");
        if (file == NULL){
            printf("Erro ao abrir o ficheiro\n");
            llCloseTx();
            return;
        }
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char controlPacket[MAX_PAYLOAD_SIZE];
        int idx = 0;

        controlPacket[idx++] = PACKET_START;
        controlPacket[idx++] = 0;
        controlPacket[idx++] = 4;
        controlPacket[idx++]= (fileSize >> 24) & 0xFF;
        controlPacket[idx++]=(fileSize >> 16) & 0xFF;
        controlPacket[idx++]= (fileSize >> 8) & 0xFF;
        controlPacket[idx++] = fileSize & 0xFF;

        int nameLenght = strlen(filename);

        controlPacket[idx++] = 1;
        controlPacket[idx++] = nameLenght;
        memcpy(&controlPacket[idx], filename, nameLenght);
        idx += nameLenght;

        if (llSend(controlPacket, idx) < 0){
            printf("Não foi possivel enviar\n");
            fclose(file);
            llCloseTx();
            return;
        }
        unsigned char dataPacket[MAX_PAYLOAD_SIZE];

        while (1) {
            int bytesRead = fread(&dataPacket[3], 1, MAX_PAYLOAD_SIZE - 3, file);
            if (bytesRead == 0) break;         

            dataPacket[0] = PACKET_DATA;
            dataPacket[1] = (bytesRead >> 8) & 0xFF;          
            dataPacket[2] = (bytesRead ) & 0xFF;   

            if (llSend(dataPacket, 3 + bytesRead ) < 0) {
                printf("Não foi possivel enviar\n");
                fclose(file);
                llCloseTx();
                return;
            }
        }
        controlPacket[0] = PACKET_END;
        if (llSend(controlPacket, idx) < 0){
            printf("Não foi possivel enviar\n");
            fclose(file);
            llCloseTx();
            return;
        }

        fclose (file);
        llCloseTx;

    }
    else if (strcmp(role, "rx") == 0)
    {
        llOpenRx(llParameters);
        
        unsigned char packet[MAX_PAYLOAD_SIZE];
        FILE *file;

        int transfer_active = 1;
        while (transfer_active) {
            int bytes_read = llReceive(packet);
            if (bytes_read < 0) continue;

            if (packet[0] == PACKET_START) {
                int pos = 1;
                char received_filename[256] = {0};

                while (pos < bytes_read) {
                    unsigned char type = packet[pos++];
                    unsigned char length = packet[pos++];

                    if (type == 0) pos += length;
                    else if (type == 1) {
                        memcpy(received_filename, &packet[pos], length);
                        received_filename[length] = '\0';
                        pos += length;
                    }
                }

                file = fopen(received_filename, "wb");

                if (file == NULL) {
                    printf("Erro ao criar o ficheiro!\n");
                    return;
                }
            } 

            else if (packet[0] == PACKET_DATA) {
                if (file != NULL) {
                    int data_size = packet[1] * 256 + packet[2];
                    
                    fwrite(&packet[3], 1, data_size, file);
                }
            }

            else if (packet[0] == PACKET_END) {
                transfer_active = 0;
            }
        }
        if (file != NULL) fclose(file); 

        llCloseRx();
    }
    else
    {
        printf("Invalid role: %s. Must be 'tx' or 'rx'.\n", role);
        return;
    }
}
