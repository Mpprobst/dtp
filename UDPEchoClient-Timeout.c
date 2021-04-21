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
    char buffer[240]="The University of Kentucky is a public, research-extensive,land grant university dedicated to improving people’s lives throughexcellence in teaching, research, health care, cultural enrichment,and economic development.";

    // base and last_sent indicate the boundaries of the sliding window
    int base = 0;         // smallest sequence number in the window
    int last_sent = -1;   // largest sequence number in the window that can be sent or has been sent. base+4
    int retransmit = 0;   // if 1, we are retransmitting a packet due to it being dropped

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

    /* loop until all packets have been sent */
    while (1==1) {
      struct data_pkt_t pkt;
      pkt.type = 1;
      pkt.seq_no = last_sent + 1;
      pkt.length = 10;

      // send the next packet if we can
      if (last_sent < base + window_size && last_sent < 23) {
        if (retransmit == 0) {
          last_sent = pkt.seq_no;
        }
        else {
          pkt.seq_no = base;
        }
        printf("SEND PACKET %i \n", pkt.seq_no);

        memcpy(pkt.data, &buffer[pkt.seq_no * 10], pkt.length);
        if (sendto(sock, &pkt, PKT_SIZE, 0, (struct sockaddr *)
                   &echoServAddr, sizeof(echoServAddr)) != PKT_SIZE)
            DieWithError("sendto() sent a different number of bytes than expected");

        if (last_sent > 23)
          last_sent = 23;
      }

      retransmit = 0;

      /* Get a response */
      struct ack_pkt_t ack;
      ack.ack_no = -2;
      fromSize = sizeof(fromAddr);
      alarm(TIMEOUT_SECS);        /* Set the timeout */
      while ((respLen = recvfrom(sock, &ack, ACK_SIZE, 0,
             (struct sockaddr *) &fromAddr, &fromSize)) < 0)
          if (errno == EINTR)     /* Alarm went off  */
          {
            // retransmit all packets from base to last_sent.
            printf("\t---- TIMEOUT ----\n");
            last_sent = base-1;   // -1 is because last_sent is incramented
            break;
          }
          else
              DieWithError("recvfrom() failed");

      /* ----- TODO: HANDLE ACK ----- */
      if (ack.ack_no < base-1) {
        // ignore
        continue;
      }

      printf("\t-------- RECEIVE ACK %i\n", ack.ack_no);
      alarm(0);
      if (ack.ack_no == 23){            // 23 is the last packet, we are done
        break;
      }
      else if (ack.ack_no == base-1) {  // ack is a duplicate
        ndups++;
        if (ndups > 3) {
          ndups = 0;
          retransmit = 1;
        }
      }
      else if (ack.ack_no >= base) {    // new ack
        ndups = 0;
        base = ack.ack_no + 1;
      }
    }
    close(sock);
    exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}
