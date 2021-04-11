#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "packet.c"
#include "ack.c"
#define ECHOMAX 255     /* Longest string to echo */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    int packet_rcvd[24];       // record of the packets received
    int * drop_list;                 // list of packets to drop

    if (argc < 3)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT> <LIST OF NUMBERS>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    // fill in the drop list
    if (argc >= 3)
    {
      int ndrops = argc - 2;
      drop_list = (int*)malloc(ndrops,sizeof(int));
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
        if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        /* ----- TODO: READ DATA ----- */
        // Packet received, determine whether to drop it or not
        // printf("RECEIVE PACKET %i", sequence_no)
        // if sequence_no == drop_list[0], then drop packet, don't send an ACK.
          // Then remove sequence_no from drop_list
          // printf("---- DROP %i", sequence_no);
        // else, packet_rcvd[sequence_no] = 1
          // copy the data in the packet to the correct loation of the receiving buffer
          // send an ACK with ack_no = max_seq
            // aka loop through packets_rcvd until seeing a 0
          // if ack_no == 23, print out the whole message in receiving buffer
          // send the ack using ack_pkt_t
          // printf("-------- SEND ACK %i", ack_no);


        // TODO: send and ACK?
        /* Send received datagram back to the client */
        if (sendto(sock, echoBuffer, recvMsgSize, 0,
             (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize)
            DieWithError("sendto() sent a different number of bytes than expected");
    }
    /* NOT REACHED */
}
