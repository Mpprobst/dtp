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
#define MAXTRIES        1       /* Tries before giving up */
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
    int last_sent = -1;   // largest sequence number in the window that can be sent or has been sent. base+4

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
    while (1==1) {
      // initially send the first 5 packets
      struct data_pkt_t pkt;
      pkt.type = 1;
      pkt.seq_no = last_sent + 1;
      pkt.length = 10;
      //printf("Sending: %s. size of = %i, %i, %i, %i = %i\n", pkt.data, sizeof(pkt.type), sizeof(pkt.seq_no), sizeof(pkt.length), sizeof(pkt.data), sizeof(pkt));

      // send the next packet if we can
      if (last_sent < base + window_size && last_sent < 23) {
        // only copy the data into the packet if we are actually ready to send
        memcpy(pkt.data, &buffer[pkt.seq_no * 10], pkt.length);

        if (sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *)
                   &echoServAddr, sizeof(echoServAddr)) != PKT_SIZE)
            DieWithError("sendto() sent a different number of bytes than expected");
        //printf("send size = %i. PKT_SIZE = %i\n", sendsize, PKT_SIZE);
        printf("SEND PACKET %i", pkt.seq_no);
        last_sent = pkt.seq_no;
        if (last_sent > 23){
          last_sent = 23;
        }
      }

      /* Get a response */
      struct ack_pkt_t ack;
      fromSize = sizeof(fromAddr);
      alarm(TIMEOUT_SECS);        /* Set the timeout */
      while ((respLen = recvfrom(sock, &ack, ACK_SIZE, 0,
             (struct sockaddr *) &fromAddr, &fromSize)) < 0)
          if (errno == EINTR)     /* Alarm went off  */
          {
            // TODO: retransmit all packets from base to last_sent.
            base = pkt.seq_no;
            last_sent = pkt.seq_no-1;   // -1 is because last_sent is incramented
            DieWithError("No Response");
          }
          else
              DieWithError("recvfrom() failed");

      /* ----- TODO: HANDLE ACK ----- */
      printf("-------- RECEIVE ACK %i\n", ack.ack_no);
      // Step1 - clear the timer
      alarm(0);
      // Step2 - process the ACK
      if (ack.ack_no == 23){          // 23 is the last packet, we are done
        break;
      }
      else if (ack.ack_no < base-1) {
        // ignore the packet
      }
      else if (ack.ack_no == base-1) {  // ack is a duplicate
        ndups++;
        if (ndups >= 3) {
          last_sent = base-1;
          printf("retransmitting pkt %i", last_sent);
          // retransmit the packet with sequence number base ONLY
          /*
          pkt.seq_no = base;
          if (sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *)
                     &echoServAddr, sizeof(echoServAddr)) != PKT_SIZE)
              DieWithError("sendto() sent a different number of bytes than expected");
          //printf("send size = %i. PKT_SIZE = %i\n", sendsize, PKT_SIZE);
          printf("SEND PACKET %i", pkt.seq_no);*/
        }
      }
      else if (ack.ack_no >= base) {    // new ack
        ndups = 0;
        base = ack.ack_no + 1;
      }
        // send new packets allowed by window size. handled at beginning of loop
          // Packets to send are packets with sequence_no from last_sent + 1 to min(base+4, 23)
          // last_sent = min(base+4, 23)

      // Step3 - set timer after processing ACK

    }

    close(sock);
    exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}
