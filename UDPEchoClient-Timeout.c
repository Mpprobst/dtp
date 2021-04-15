#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */
#include "packet.c"
#include "ack.c"

#define TIMEOUT_SECS    4       /* Seconds between retransmits */
#define MAXTRIES        5       /* Tries before giving up */
#define PKT_SIZE        22
#define ACK_SIZE        8

int tries=0;   /* Count of times sent - GLOBAL for signal-handler access */
const int window_size = 5;   // const

//void DieWithError(char *errorMessage);   /* Error handling function */
void CatchAlarm(int ignored);            /* Handler for SIGALRM */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sigaction myAction;       /* For setting signal handler */
    char *servIP;                    /* IP address of server */
    int respLen;                     /* Size of received datagram */

    // buffer is ALL the data we are sending to the server
    // packets can take 10B data at a time. 1 char = 1B, therefore we need 24 packets to transmit this message
    char buffer[240]="The University of Kentucky is a public, research-extensive,land grant university dedicated to improving people’s lives throughexcellence in teaching, research, health care, cultural enrichment,and economic development.";

    // base and last_sent indicate the boundaries of the sliding window
    int base = 0;        // smallest sequence number in the window
    int last_sent = 0;   // largest sequence number in the window that can be sent or has been sent. base+4

    int ndups = 0;   // number of duplicate ACKs received

    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];           /* First arg:  server IP address (dotted quad) */

    if (argc == 4)
        echoServPort = atoi(argv[3]);  /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is well-known port for echo service */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);       /* Server port */

    /* ----- TODO: loop until all packets have been sent ----- */
    // initially send the first 5 packets
    struct data_pkt_t pkt;
    pkt.type = 1;
    pkt.seq_no = 0;
    pkt.length = 10;
    memcpy(pkt.data, &buffer[pkt.seq_no * 10], pkt.length);
    //printf("Sending: %s. size of = %i, %i, %i, %i = %i\n", pkt.data, sizeof(pkt.type), sizeof(pkt.seq_no), sizeof(pkt.length), sizeof(pkt.data), sizeof(pkt));

    // printf("SEND PACKET %i", packet_no)
    /* Send the string to the server */
    int sendsize = sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));
    //if (sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *)
    //           &echoServAddr, sizeof(echoServAddr)) != PKT_SIZE)
    //    DieWithError("sendto() sent a different number of bytes than expected");
    //printf("send size = %i. PKT_SIZE = %i\n", sendsize, PKT_SIZE);
    if (sendsize != PKT_SIZE){
      DieWithError("sendto() sent a different number of bytes than expected");
    }

    /* Get a response */
    struct ack_pkt_t ack_pkt;
    fromSize = sizeof(fromAddr);
    alarm(TIMEOUT_SECS);        /* Set the timeout */
    while ((respLen = recvfrom(sock, &ack_pkt, ACK_SIZE, 0,
           (struct sockaddr *) &fromAddr, &fromSize)) < 0)
        if (errno == EINTR)     /* Alarm went off  */
        {
            if (tries < MAXTRIES)      /* incremented by signal handler */
            {
                printf("timed out, %d more tries...\n", MAXTRIES-tries);
                if (sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *)
                            &echoServAddr, sizeof(echoServAddr)) != PKT_SIZE)
                    DieWithError("sendto() failed");
                alarm(TIMEOUT_SECS);
            }
            else
            {
                // TODO: retransmit all packets from base to last_sent.
                // set a timer
                DieWithError("No Response");
            }
        }
        else
            DieWithError("recvfrom() failed");

    /* ----- TODO: HANDLE ACK ----- */
    printf("-------- RECEIVE ACK %i\n", ack_pkt.ack_no);
    // Step1 - clear the timer
    alarm(0);
    // Step2 - process the ACK
    // if ack_no == 23, then stop
    // if ack_no < base-1, then ignore
    // if ack_no == base-1, then it is a duplicate
    // if num_dups >= 3, then retransmit the packet with sequence number base ONLY
    // if ack_no >= base, then we have a new ACK.
      // set ndups = 0.
      // base = ack_no + 1.
      // send new packets allowed by window size.
        // Packets to send are packets with sequence_no from last_sent + 1 to min(base+4, 23)
        // last_sent = min(base+4, 23)

    // Step3 - set timer after processing ACK

    /* null-terminate the received data */
    //echoBuffer[respStringLen] = '\0';
    //printf("Received: %s\n", echoBuffer);    /* Print the received data */

    close(sock);
    exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}
