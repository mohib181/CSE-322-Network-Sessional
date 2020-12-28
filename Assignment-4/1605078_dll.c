#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

#define DATA 0
#define ACK 1
#define PIGGY 2

#define PAYLOAD_SIZE 4

/* a "pkt" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct pkt
{
    char data[PAYLOAD_SIZE];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct frm
{
    int type;
    int seqnum;
    int acknum;
    int checksum;
    char payload[PAYLOAD_SIZE];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct frm frame);
void tolayer5(int AorB, char datasent[PAYLOAD_SIZE]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int piggyback;
int crc;
char polynomial[32];

int A_send_seq;
int A_receive_seq;
int B_send_seq;
int B_receive_seq;

int A_send_ack;
int A_receive_ack;
int B_send_ack;
int B_receive_ack;

int A_ok;
int B_ok;

int A_waiting_ack;
int B_waiting_ack;

int A_has_waiting_ack;
int B_has_waiting_ack;

struct frm A_frame;
struct frm B_frame;

char ack_data[] = {'1', '1', '1', '1'};
char nack_data[] = {'0', '0', '0', '0'};

int getRemainderCRC(struct frm fm, int appendCheckSum) {
    int i = 0;
    char dataBlock[70];

    if(fm.type == DATA) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '0';
    } else if (fm.type == ACK) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '1';
    } else if (fm.type == PIGGY) {
        dataBlock[i++] = '1';
        dataBlock[i++] = '0';
    } else {
        dataBlock[i++] = '1';
        dataBlock[i++] = '1';
    }

    if (fm.acknum == 0) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '0';
    } else if (fm.acknum == 1) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '1';
    } else {
        dataBlock[i++] = '1';
        dataBlock[i++] = '1';
    }

    if (fm.seqnum == 0) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '0';
    } else if (fm.seqnum == 1) {
        dataBlock[i++] = '0';
        dataBlock[i++] = '1';
    } else {
        dataBlock[i++] = '1';
        dataBlock[i++] = '1';
    }

    int k = 0;
    char binary[32];
    for (int j = 0; j < PAYLOAD_SIZE; ++j) {
        int a = (int)fm.payload[j];
        while (a > 0) {
            binary[k++] = (a%2)? '1' : '0';
            a /= 2;
        }
    }
    for (int j = k-1; j >= 0; --j, i++) {
        dataBlock[i] = binary[j];
    }
    dataBlock[i] = '\0';
    if(crc == 1 && appendCheckSum == 1) printf("\nReceiver Side:\n");
    if(crc == 1 && appendCheckSum == 0) printf("\nSender Side:\n");
    if(crc == 1) printf("data bit string: %s\n", dataBlock);

    if (appendCheckSum == 1) {
        for (int p = 0; p < strlen(polynomial)-1; ++p) {
            binary[p] = '0';
        }
        k = 0;
        int a = fm.checksum;
        while (a > 0) {
            binary[k++] = (a%2)? '1' : '0';
            a /= 2;
        }
        for (int j = strlen(polynomial)-2; j >= 0; --j, i++) {
            dataBlock[i] = binary[j];
        }
    }
    else {
        for (int j = 0; j < strlen(polynomial)-1; ++j, ++i) {
            dataBlock[i] = '0';
        }
    }
    dataBlock[i] = '\0';
    if(crc == 1) {
        printf("polynomial: %s\n", polynomial);
        printf("Input bit string: %s\n", dataBlock);
    }

    //performing modulo division
    int index = 0;
    int poly_len = (int)strlen(polynomial);
    int data_len = (int)strlen(dataBlock);
    while(index <= data_len-poly_len) {
        if (dataBlock[index] == '1') {
            for (int p = 0; p < poly_len; p++) {
                if (polynomial[p] == dataBlock[index+p]) dataBlock[index+p] = '0';
                else dataBlock[index+p] = '1';
            }
        }
        else index++;
    }

    //getting the value of remainder
    char remainder[32];
    int r = 0;
    for (r = 0; r < poly_len-1; ++r) {
        remainder[r] = dataBlock[index+r];
    }
    remainder[r] = '\0';

    int val = 0, multiplier = 0;
    for (int m = r-1; m >= 0; --m, ++multiplier) {
        val += (remainder[m]%48)*(1<<multiplier);
    }
    if (crc == 1) printf("remainder: %s, value: %d\n", remainder, val);
    return val;
}

void printFrameContent(struct frm fm) {
    printf("type: %d, ack: %d, seq: %d, check: %d, payload: ", fm.type, fm.acknum, fm.seqnum, fm.checksum);
    for (int i = 0; i < PAYLOAD_SIZE; ++i) {
        printf("%c", fm.payload[i]);
    }
    printf("\n");
}

int calculateCheckSum(struct frm fm) {
    //int rem = getRemainderCRC(fm, 0);
    int check = fm.type + fm.acknum + fm.seqnum;
    for (int i = 0; i < PAYLOAD_SIZE; ++i) {
        check += fm.payload[i];
    }
    return check;
}

int checkCorrupted(struct frm fm) {
    int isCorrupted = 1;

    /*int check = calculateCheckSum(fm);
    if (check == fm.checksum) isCorrupted = 0;*/

    int check = getRemainderCRC(fm, 1);
    if (check == 0) isCorrupted = 0;
    if (crc == 1 && check != 0) printf("Error occurred\n");

    return isCorrupted;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct pkt packet)
{
    if(A_ok == 0){
        printf("A_output: A packet is already waiting to be acknowledged\ndropping this one\n");
        return;
    }

    if (A_has_waiting_ack == 1 && piggyback == 1) {
        A_frame.type = PIGGY;
        A_frame.acknum = A_waiting_ack;

        A_has_waiting_ack = 0;
    } else {
        A_frame.type = DATA;
        A_frame.acknum = 0;
    }
    A_frame.seqnum = A_send_seq;
    strcpy(A_frame.payload, packet.data);
    //A_frame.checksum = calculateCheckSum(A_frame);
    A_frame.checksum = getRemainderCRC(A_frame, 0);

    printf("A_output ");
    printFrameContent(A_frame);
    //makeDataBlock(A_frame);

    starttimer(0, 50.0);
    tolayer3(0, A_frame);
    A_ok = 0;
}

/* need be completed only for extra credit */
void B_output(struct pkt packet)
{
    if(B_ok == 0){
        printf("B_output: A packet is already waiting to be acknowledged\ndropping this one\n");
        return;
    }

    if (B_has_waiting_ack == 1 && piggyback == 1) {
        B_frame.type = PIGGY;
        B_frame.acknum = B_waiting_ack;

        B_has_waiting_ack = 0;
    } else {
        B_frame.type = DATA;
        B_frame.acknum = 0;
    }
    B_frame.seqnum = B_send_seq;
    strcpy(B_frame.payload, packet.data);
    //B_frame.checksum = calculateCheckSum(B_frame);
    B_frame.checksum = getRemainderCRC(B_frame, 0);

    printf("B_output ");
    printFrameContent(B_frame);

    starttimer(1, 50.0);
    tolayer3(1, B_frame);
    B_ok = 0;
}

/* called from layer 3, when a frame arrives for layer 4 */
void A_input(struct frm frame)
{
    int isCorrupted = checkCorrupted(frame);

    if (frame.type == DATA) {
        if (isCorrupted == 0) {
            if (frame.seqnum == A_receive_seq) {
                printf("A_input: proper data frame from B received\n");
                printFrameContent(frame);
                tolayer5(0, frame.payload);

                A_receive_seq ^= 1;
                if (piggyback == 1) {
                    A_waiting_ack = A_send_ack;
                    A_has_waiting_ack = 1;
                }
                else {
                    frame.type = ACK;
                    frame.acknum = A_send_ack;
                    strcpy(frame.payload, ack_data);
                    //frame.checksum = calculateCheckSum(frame);
                    frame.checksum = getRemainderCRC(frame, 0);

                    printf("Sending ACK: ");
                    printFrameContent(frame);

                    tolayer3(0, frame);
                }
                A_send_ack ^= 1;

                /*frame.type = ACK;
                frame.acknum = A_send_ack;
                frame.seqnum = 0;
                printf("Sending ACK: type: %d, ack: %d, seq: %d, check: %d\n", frame.type, frame.acknum, frame.seqnum, frame.checksum);
                tolayer3(0, frame);

                A_receive_seq ^= 1;
                A_send_ack ^= 1;*/
            }
            else {
                printf("A_input: duplicate frame ");
                printFrameContent(frame);

                frame.type = ACK;
                if (piggyback == 1) {
                    frame.acknum = A_waiting_ack;
                    A_has_waiting_ack = 0;
                }
                else {
                    frame.acknum = A_send_ack^1;
                }
                strcpy(frame.payload, ack_data);
                //frame.checksum = calculateCheckSum(frame);
                frame.checksum = getRemainderCRC(frame, 0);

                if(piggyback == 1) printf("Sending ACK(outstanding ack): ");
                else printf("Sending ACK: ");
                printFrameContent(frame);

                tolayer3(0, frame);
            }
        }
        else {
            printf("A_input: corrupted frame ");
            printFrameContent(frame);

            frame.type = ACK;
            frame.acknum = A_send_ack ^ 1;
            strcpy(frame.payload, nack_data);
            //frame.checksum = calculateCheckSum(frame);
            frame.checksum = getRemainderCRC(frame, 0);

            printf("Sending NACK(last ack): ");
            printFrameContent(frame);

            tolayer3(0, frame);
        }
    }
    else if (frame.type == ACK) {
        if (isCorrupted == 0 && frame.acknum == A_receive_ack) {
            printf("A_input: acknowledgement received from B\n");
            printFrameContent(frame);

            stoptimer(0);

            A_receive_ack = A_receive_ack^1;
            A_send_seq = A_send_seq^1;
            A_ok = 1;
        } else {
            printf("A_input: corrupted or ack num diff: not acknowledged\n");
            printFrameContent(frame);
        }
    }
    else {
        if (isCorrupted == 0) {
            if (frame.seqnum == A_receive_seq) {
                printf("A_input: proper piggyback data frame received from B\n");
                printFrameContent(frame);

                tolayer5(0, frame.payload);

                A_receive_seq ^= 1;

                A_waiting_ack = A_send_ack;
                A_has_waiting_ack = 1;
                A_send_ack ^= 1;
            }
            else {
                printf("A_input: duplicate piggyback frame ");
                printFrameContent(frame);

                frame.type = ACK;
                frame.acknum = A_waiting_ack;
                strcpy(frame.payload, ack_data);
                //frame.checksum = calculateCheckSum(frame);
                frame.checksum = getRemainderCRC(frame, 0);

                printf("Sending ACK(outstanding ack): ");
                printFrameContent(frame);

                tolayer3(0, frame);

                A_has_waiting_ack = 0;
            }

            if (frame.acknum == A_receive_ack) {
                printf("A_input: piggyback acknowledgement received from B\n");
                printFrameContent(frame);

                stoptimer(0);

                A_receive_ack = A_receive_ack^1;
                A_send_seq = A_send_seq^1;
                A_ok = 1;
            } else {
                printf("A_input: piggyback ack num diff: not acknowledged\n");
                printFrameContent(frame);
            }
        }
        else {
            printf("A_input: corrupted piggyback frame ");
            printFrameContent(frame);

            frame.type = ACK;
            frame.acknum = A_send_ack ^ 1;
            strcpy(frame.payload, nack_data);
            //frame.checksum = calculateCheckSum(frame);
            frame.checksum = getRemainderCRC(frame, 0);

            printf("Sending NACK(last ack): ");
            printFrameContent(frame);

            tolayer3(0, frame);
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    printf("A_timerinterrupt:\nResending: ");
    printFrameContent(A_frame);

    tolayer3(0, A_frame);
    starttimer(0, 50.0);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    A_send_ack = 0;
    A_receive_ack = 0;
    A_send_seq = 0;
    A_receive_seq = 0;

    A_has_waiting_ack = 0;
    A_ok = 1;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a frame arrives for layer 4 at B*/
void B_input(struct frm frame)
{
    int isCorrupted = checkCorrupted(frame);

    if (frame.type == DATA) {
        if (isCorrupted == 0) {
            if (frame.seqnum == B_receive_seq) {
                printf("B_input: proper data frame received from A\n");
                printFrameContent(frame);

                tolayer5(1, frame.payload);

                B_receive_seq ^= 1;
                if (piggyback == 1) {
                    B_waiting_ack = B_send_ack;
                    B_has_waiting_ack = 1;
                }
                else {
                    frame.type = ACK;
                    frame.acknum = B_send_ack;
                    strcpy(frame.payload, ack_data);
                    //frame.checksum = calculateCheckSum(frame);
                    frame.checksum = getRemainderCRC(frame, 0);

                    printf("Sending ACK: ");
                    printFrameContent(frame);

                    tolayer3(1, frame);
                }
                B_send_ack ^= 1;

                /*frame.type = ACK;
                frame.acknum = B_send_ack;
                frame.seqnum = 0;
                printf("Sending ACK: type: %d, ack: %d, seq: %d, check: %d\n", frame.type, frame.acknum, frame.seqnum, frame.checksum);
                tolayer3(1, frame);

                B_receive_seq ^= 1;
                B_send_ack ^= 1;*/
            } else {
                printf("B_input: duplicate frame ");
                printFrameContent(frame);

                frame.type = ACK;
                if (piggyback == 1) {
                    frame.acknum = B_waiting_ack;
                    B_has_waiting_ack = 0;
                }
                else {
                    frame.acknum = B_send_ack^1;
                }
                strcpy(frame.payload, ack_data);
                //frame.checksum = calculateCheckSum(frame);
                frame.checksum = getRemainderCRC(frame, 0);

                if(piggyback == 1) printf("Sending ACK(outstanding ack): ");
                else printf("Sending ACK: ");
                printFrameContent(frame);

                tolayer3(1, frame);
            }
        } else {
            printf("B_input: corrupted frame ");
            printFrameContent(frame);

            frame.type = ACK;
            frame.acknum = B_send_ack ^ 1;
            strcpy(frame.payload, nack_data);
            //frame.checksum = calculateCheckSum(frame);
            frame.checksum = getRemainderCRC(frame, 0);

            printf("Sending NACK(last ack):  ");
            printFrameContent(frame);

            tolayer3(1, frame);
        }

    }
    else if (frame.type == ACK) {
        if (isCorrupted == 0 && frame.acknum == B_receive_ack) {
            printf("B_input: acknowledgement received from A\n");
            printFrameContent(frame);

            stoptimer(1);

            B_receive_ack = B_receive_ack^1;
            B_send_seq = B_send_seq^1;
            B_ok = 1;
        } else {
            printf("B_input: corrupted or ack num diff: not acknowledged\n");
            printFrameContent(frame);
        }
    }
    else {
        if (isCorrupted == 0) {
            if (frame.seqnum == B_receive_seq) {
                printf("B_input: proper piggyback data frame received from A\n");
                printFrameContent(frame);
                tolayer5(1, frame.payload);

                B_receive_seq ^= 1;

                B_waiting_ack = B_send_ack;
                B_has_waiting_ack = 1;
                B_send_ack ^= 1;
            } else {
                printf("B_input: duplicate piggyback frame ");
                printFrameContent(frame);

                frame.type = ACK;
                frame.acknum = B_waiting_ack;
                strcpy(frame.payload, ack_data);
                //frame.checksum = calculateCheckSum(frame);
                frame.checksum = getRemainderCRC(frame, 0);

                printf("Sending ACK(Outstanding ack): ");
                printFrameContent(frame);

                tolayer3(1, frame);

                B_has_waiting_ack = 0;
            }

            if (frame.acknum == B_receive_ack) {
                printf("B_input: piggyback acknowledgement received from A\n");
                printFrameContent(frame);

                stoptimer(1);

                B_receive_ack = B_receive_ack^1;
                B_send_seq = B_send_seq^1;
                B_ok = 1;
            } else {
                printf("B_input: piggyback ack num diff: not acknowledged\n");
                printFrameContent(frame);
            }
        } else {
            printf("B_input: corrupted piggyback frame ");
            printFrameContent(frame);

            frame.type = ACK;
            frame.acknum = B_send_ack ^ 1;
            strcpy(frame.payload, nack_data);
            //frame.checksum = calculateCheckSum(frame);
            frame.checksum = getRemainderCRC(frame, 0);

            printf("Sending NACK(last ack): ");
            printFrameContent(frame);

            tolayer3(1, frame);
        }
    }
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    printf("B_timerinterrupt:\nResending: ");
    printFrameContent(B_frame);

    tolayer3(1, B_frame);
    starttimer(1, 50.0);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    B_send_ack = 0;
    B_receive_ack = 0;
    B_send_seq = 0;
    B_receive_seq = 0;
    B_has_waiting_ack = 0;
    B_ok = 1;
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct frm *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    struct event *eventptr;
    struct pkt msg2give;
    struct frm pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in pkt to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 4; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[3] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.type = eventptr->pktptr->type;
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 4; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

    terminate:
    printf(
            " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
            time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d",&nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f",&lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f",&corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Enter crc steps:");
    scanf("%d",&crc);
    printf("Enter piggyback:");
    scanf("%d",&piggyback);
    printf("Enter generator polynomial:");
    scanf("%s",&polynomial);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct frm packet)
{
    struct frm *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct frm *)malloc(sizeof(struct frm));
    mypktptr->type = packet.type;
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 4; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 4; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}
