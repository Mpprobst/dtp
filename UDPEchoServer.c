#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "packet.c"
#include "ack.c"
#include "string.h"

#define PKT_SIZE        22
#define ACK_SIZE        8

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    unsigned short echoServPort;     /* Server port */
    struct data_pkt_t pkt;
    int recvMsgSize;                 /* Size of received message */
    const int num_packets = 24;
    const int data_length = 10;
    int packet_rcvd[num_packets] = {0};    // record of the packets received
    int * drop_list;                 // list of packets to drop
    int ndrops = 0;                  // length of drop_list
    int drop_idx = 0;                // index of next packet to drop
    char * buffer[data_length * num_packets];

    if (argc < 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT> <LIST OF NUMBERS>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    // fill in the drop list
    if (argc >= 3)
    {
      ndrops = argc - 2;
      drop_list = (int*)malloc(ndrops*sizeof(int));
      for (int i = 0; i < ndrops; i++)
      {
        drop_list[i] = atoi(argv[i+2]);
      }
    }

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    //printf("serv: %i", echoServAddr.sin_addr.s_addr);

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, &pkt, PKT_SIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        //fprintf(stderr, "Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        /* ----- TODO: READ DATA ----- */
        fprintf(stderr, "RECEIVE PACKET %i\n", pkt.seq_no);
        if (ndrops > drop_idx) {
          if (pkt.seq_no == drop_list[drop_idx]) {
            fprintf(stderr, "---- DROP %i", pkt.seq_no);
            drop_idx++;
          }
        }
        else {
          packet_rcvd[pkt.seq_no] = 1;
          int data_idx = pkt.seq_no * data_length * sizeof(char);
          //fprintf(stderr, "insert into buffer[%i]\n", pkt.seq_no * data_length);
          //memcpy(buffer + data_idx, pkt.data, pkt.length * sizeof(char));
          strcpy(buffer + data_idx, pkt.data);
          fprintf(stderr, "buff[%i]=%s\n", data_idx, buffer + data_idx);

          struct ack_pkt_t ack;
          ack.type = 2;
          for (int i = 0; i < num_packets; i++) {
            ack.ack_no = i;
            if (packet_rcvd[i] == 0){
              ack.ack_no--;
              break;
            }
          }

          if (ack.ack_no < num_packets) {
            fprintf(stderr, "-------- SEND ACK %i\n", ack.ack_no);
            if (sendto(sock, &ack, ACK_SIZE, 0,
                 (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != ACK_SIZE)
                DieWithError("sendto() sent a different number of bytes than expected");

            if (ack.ack_no == num_packets-1) {
              for (int i = 0; i < num_packets ; i++){
                fprintf(stderr, "%s", buffer + (i * data_length * sizeof(char)));
              }
              fprintf(stderr, "\n");
              return 0;
            }
          }
        }
    }
    /* NOT REACHED */
}
