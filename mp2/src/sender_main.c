/*
 * File:   sender_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

struct sockaddr_in si_other;
int s;
socklen_t slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

typedef struct segmentT {
  unsigned long long int seq_num;
  char app_message[];
} segment;

typedef struct packet_time_info {
  struct timeval time_sent;
  long long int timeout_interval;
} packet_time;

long long int MSS = 1472;
long long int CWND = 1472;
long long int SSTHRESH = 64000;
int DUP_COUNT = 0;
long long int NEXT_BYTE_TO_SEND = 1;
long long int LAST_ACK_RECV = 1;
long long int TIMEOUT_INTERVAL = 1;
long long int SAMPLE_RTT = 20;
long long int ESTIMATED_RTT = 20;
long long int DEV_RTT = 0;
long long int CWND_START = 1;
char STATE = 'S';
segment** SEND_BUFFER; // array of segments
int BUFFER_IDX = 0;

packet_time** SENT_TIMES; // array of packet_time elems for when that segment will timeout and when it was sent
int idx_timer_started = -1;
int num_pkts_acked = 0;
struct timeval timeout_struct;

/* Helper function to return absolute value */
long long int abs_val(long long int a) {
  if (a < 0) return a * -1;
  return a;
}

/* Helper function to return current time */
struct timeval currentTime() {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  //long long int time = (long long int)tv.tv_sec * 1000 + ((long long int)tv.tv_usec) / 1000;
  return current_time;
}

/* Return 1 if there's a timeout, otherwise return 0  */
int checkTimeout() {

  //fprintf(stderr, "entering checkTimeout()\n");

  struct timeval current_time_struct = currentTime(); // current time
  long long int current_time = (long long int)current_time_struct.tv_sec * 1000 +
    ((long long int)current_time_struct.tv_usec) / 1000;

  packet_time* last_pckt_not_acked = SENT_TIMES[num_pkts_acked];

  struct timeval time_sent_not_acked = last_pckt_not_acked->time_sent;
  long long int packet_time_timeout = (long long int)time_sent_not_acked.tv_sec * 1000 +
    ((long long int)time_sent_not_acked.tv_usec) / 1000 + 40000;

    //fprintf(stderr, "after the timeout calculations\n");

  if (current_time >= packet_time_timeout) return 1;

  //fprintf(stderr, "NO TIMEOUT\n");
  return 0;
}

/* Helper function to retransmit packet */
void retransmitPacket(int socket, unsigned long long int bytesToTransfer, char* filename) {
  // send with seq_num = LAST_ACK_RECV

  //fprintf(stderr, "ENTERING RETRANSMIT PACKET\n");

  packet_time* curr_packet_info = SENT_TIMES[num_pkts_acked];

  if (idx_timer_started == -1) { // timer is not set
    curr_packet_info->time_sent = currentTime();
    curr_packet_info->timeout_interval = TIMEOUT_INTERVAL;

    // start timer with this timeout interval:
    timeout_struct.tv_sec = TIMEOUT_INTERVAL;
    timeout_struct.tv_usec = 0;
    idx_timer_started = num_pkts_acked;
  }

  else { // timer has already been started:
    packet_time* p = SENT_TIMES[num_pkts_acked - 1];
    curr_packet_info->time_sent.tv_sec = currentTime().tv_sec - p->time_sent.tv_sec;
    curr_packet_info->time_sent.tv_usec = currentTime().tv_usec - p->time_sent.tv_usec;
    curr_packet_info->timeout_interval = TIMEOUT_INTERVAL;
    //SENT_TIMES[BUFFER_IDX].tv_sec = (currentTime().tv_sec - SENT_TIMES[BUFFER_IDX - 1].tv_sec) + TIMEOUT_INTERVAL ;
    //SENT_TIMES[BUFFER_IDX].tv_usec = currentTime().tv_usec - SENT_TIMES[BUFFER_IDX - 1].tv_usec;
  }
  //fprintf(stderr, "after all the dumb time stuff\n");

  FILE *fp_temp;
  fp_temp = fopen(filename, "rb");
  if (fp_temp == NULL) {
      printf("Could not open file to send.");
      exit(1);
  }
  fseek(fp_temp, LAST_ACK_RECV - 1, 0L);
  //fprintf(stderr, "after opening and fseek\n");
  //fprintf(stderr, "LAST_ACK_RECV = %llu\n", LAST_ACK_RECV);

  unsigned long long int min = MSS;
  if (LAST_ACK_RECV + MSS > bytesToTransfer) min = bytesToTransfer - LAST_ACK_RECV + 1;

  char* app_message = malloc(min);
  memset(app_message, 0, min);
  size_t bytes_read = fread(app_message, 1, min, fp_temp);
  if (bytes_read != min) fprintf(stderr, "didn't read correct num of bytes from file\n");
  //fprintf(stderr, "after fread\n");

  char seq_num_string[256];
  ssize_t seq_num_digits = sprintf(seq_num_string, "%llu", LAST_ACK_RECV);
  //fprintf(stderr, "after getting sequence number digits\n");


  char message[bytes_read + seq_num_digits];
  //fprintf(stderr, "after creating the message char[]\n");
  memcpy(&message[0], seq_num_string, seq_num_digits); // seq number
  //fprintf(stderr, "after putting in seq num the message = %s\n", message);
  memcpy(&message[seq_num_digits], app_message, bytes_read); // app message
  //fprintf(stderr, "after putting in app message the message = %s\n", message);
  message[bytes_read + seq_num_digits] = 0;
  //fprintf(stderr, "retransmitting message = --%s--", message);
  //fprintf(stderr, "after null terminating the message\n");

  ssize_t bytes_sent = sendto(socket, message, seq_num_digits + bytes_read, 0, (struct sockaddr*)&si_other, slen);
  if (bytes_sent <= 0) perror("failed to send bytes to receiver");
  if (bytes_sent != seq_num_digits + bytes_read) fprintf(stderr, "didn't send correct num of bytes. sent = %zu\n", bytes_sent);

  fclose(fp_temp);
}



/**
* Helper function to create new segment to send to receiver
*/
void sendPacket(int socket, unsigned long long int bytesToTransfer, FILE* fp) {
  /*fprintf(stderr, "NEXT_BYTE_TO_SEND = %llu\n", NEXT_BYTE_TO_SEND);
  fprintf(stderr, "can send CWND_START = %llu\n", CWND_START);
  fprintf(stderr, "can send CWND_START + CWND - 1 = %llu\n", CWND_START + CWND - 1);
  fprintf(stderr, "BUFFER_IDX = %d\n", BUFFER_IDX);*/

  //fprintf(stderr, "ENTERING SEND PACKET\n");

  if (NEXT_BYTE_TO_SEND < CWND_START || NEXT_BYTE_TO_SEND >= CWND_START + CWND) {
    //fprintf(stderr, "sendPacket(): out of bounds of cwnd\n" );
    return;
  }
  packet_time* curr_packet_info = SENT_TIMES[BUFFER_IDX];

  if (idx_timer_started == -1) { // timer is not set
    curr_packet_info->time_sent = currentTime();
    curr_packet_info->timeout_interval = TIMEOUT_INTERVAL;

    // start timer with this timeout interval:
    timeout_struct.tv_sec = TIMEOUT_INTERVAL;
    timeout_struct.tv_usec = 0;
    idx_timer_started = BUFFER_IDX;
  }

  else { // timer has already been started:
    packet_time* p = SENT_TIMES[BUFFER_IDX - 1];

    curr_packet_info->time_sent.tv_sec = currentTime().tv_sec - p->time_sent.tv_sec;
    curr_packet_info->time_sent.tv_usec = currentTime().tv_usec - p->time_sent.tv_usec;
    curr_packet_info->timeout_interval = TIMEOUT_INTERVAL;
    //SENT_TIMES[BUFFER_IDX].tv_sec = (currentTime().tv_sec - SENT_TIMES[BUFFER_IDX - 1].tv_sec) + TIMEOUT_INTERVAL ;
    //SENT_TIMES[BUFFER_IDX].tv_usec = currentTime().tv_usec - SENT_TIMES[BUFFER_IDX - 1].tv_usec;
  }

  segment* curr_segment = SEND_BUFFER[BUFFER_IDX];

  unsigned long long int min = MSS;
  if ((MSS + NEXT_BYTE_TO_SEND) + 1 > bytesToTransfer) min = bytesToTransfer - NEXT_BYTE_TO_SEND + 1;
  //fprintf(stderr, "min is set to = %llu\n", min);

  char app_message[min];
  size_t bytes_read = fread(&app_message[0], 1, min, fp);
  if (bytes_read != min) fprintf(stderr, "didn't read correct num of bytes from file\n");

  curr_segment->seq_num = NEXT_BYTE_TO_SEND;
  memcpy(curr_segment->app_message, app_message, min);

  char seq_num_string[256];
  ssize_t seq_num_digits = sprintf(seq_num_string, "%llu", curr_segment->seq_num);
  //fprintf(stderr, "SENDING PACKET seq_num_string = --%s--\n", seq_num_string);
  //fprintf(stderr, "seq_num_digits = %zu\n", seq_num_digits);

  char message[seq_num_digits + bytes_read];
  memcpy(&message[0], seq_num_string, seq_num_digits); // seq number
  memcpy(&message[seq_num_digits], app_message, bytes_read); // app message
  //fprintf(stderr, "sending to receiver message = --%s--, length = %zu\n", message, bytes_read + seq_num_digits);


  ssize_t bytes_sent = sendto(socket, message, seq_num_digits + bytes_read, 0, (struct sockaddr*)&si_other, slen);
  timeout_struct.tv_sec = 0;
  timeout_struct.tv_usec = 40000;
  if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_struct, sizeof(timeout_struct)) < 0)
      perror("setsockopt failed\n");

  if (bytes_sent <= 0) perror("failed to send bytes to receiver");
  if (bytes_sent != seq_num_digits + bytes_read) fprintf(stderr, "didn't send correct num of bytes. sent = %zu\n", bytes_sent);


  NEXT_BYTE_TO_SEND += min;
  BUFFER_IDX++;

  //fprintf(stderr, "NEXT_BYTE_TO_SEND after incrementing = %llu\n", NEXT_BYTE_TO_SEND);
}





/**
* Helper function to receive ack from receiver
* Return 0 if duplicate ack, return 1 if new ack, return -1 if no ack
*/
int receivePacket(int socket, unsigned long long int bytesToTransfer) {
  // waiting to receive ack number = LAST_ACK_RECV + MSS or bytesToTransfer
  //fprintf(stderr, "ENTERING RECEIVE PACKET\n");

  unsigned long long int desired_ack_num = LAST_ACK_RECV + MSS;
  if (LAST_ACK_RECV + MSS > bytesToTransfer) desired_ack_num = bytesToTransfer;

  char desired_ack_string[256];
  ssize_t desired_ack_digits = sprintf(desired_ack_string, "%llu", desired_ack_num);
  //fprintf(stderr, "desired_ack_digits = %zu\n", desired_ack_digits);

  char recv_ack[desired_ack_digits];
  memset(recv_ack, 0, desired_ack_digits);

  ssize_t bytes_received = recvfrom(socket, recv_ack, desired_ack_digits, 0, (struct sockaddr*)&si_other, &slen);
  if (bytes_received <= 0) {
    fprintf(stderr, "failed to read bytes from receiver\n");
    return -1;
  }
  if (bytes_received != desired_ack_digits) fprintf(stderr, "didn't receive correct num of bytes. received = %zu\n", bytes_received);

  recv_ack[desired_ack_digits] = 0;
  //fprintf(stderr, "received ack from receiver = --%s--\n", recv_ack);
  unsigned long long int ack_num = atoll(recv_ack);
  //fprintf(stderr, "received ack from receiver = %llu\n", ack_num);

  if (ack_num >= desired_ack_num) { // cummulative ack
    LAST_ACK_RECV = ack_num;
    //fprintf(stderr, "got new ack from receiver, LAST_ACK_RECV = %llu\n", LAST_ACK_RECV);

    SAMPLE_RTT = currentTime().tv_sec - SENT_TIMES[num_pkts_acked]->time_sent.tv_sec;
    ESTIMATED_RTT = 0.875 * ESTIMATED_RTT + 0.125 * SAMPLE_RTT;
    DEV_RTT = (1- 0.25) * DEV_RTT + 0.25 * abs_val(SAMPLE_RTT - ESTIMATED_RTT);
    TIMEOUT_INTERVAL = ESTIMATED_RTT + 4 * DEV_RTT;

    if (LAST_ACK_RECV != NEXT_BYTE_TO_SEND) {
      // start timer b/c there's unacked packets with timer equal to SENT_TIMES[num_pkts_acked]
      timeout_struct.tv_sec = SENT_TIMES[num_pkts_acked]->time_sent.tv_sec;
      timeout_struct.tv_usec = SENT_TIMES[num_pkts_acked]->time_sent.tv_usec;

      timeout_struct.tv_sec = 0;
      timeout_struct.tv_usec = 40000;
      if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_struct, sizeof(timeout_struct)) < 0)
          perror("setsockopt failed\n");
    }
    num_pkts_acked++;
    return 1;
  }

  else if (ack_num == LAST_ACK_RECV) {
    //fprintf(stderr, "got duplicate ack from receiver\n");
    return 0;
  }

  else {
    //fprintf(stderr, "receivePacket()--> WHY ARE YOU PRINTING?? ack_num = %llu, desired_ack_num = %llu, LAST_ACK_RECV = %llu\n", ack_num, desired_ack_num, LAST_ACK_RECV);
    return -1;
  }

}





void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) { // convert host address into readable form by network
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


    // allocate space for SEND_BUFFER:
    int num_segments = bytesToTransfer / MSS;
    if (bytesToTransfer % MSS != 0) num_segments++;
    //fprintf(stderr, "num_segments = %d\n", num_segments);

    SEND_BUFFER = (segment**)malloc(num_segments * sizeof(segment*));
    SENT_TIMES = (packet_time**)malloc(num_segments * sizeof(packet_time*));
    for (int i = 0; i < num_segments; i++) {
      SEND_BUFFER[i] = malloc(sizeof(segment));
      SENT_TIMES[i] = malloc(sizeof(packet_time));
    }

    // start off by sending everything in the CWND (which is 1 MSS so only 1 packet)
    //fprintf(stderr, "BEFORE sending initial packets\n");
    sendPacket(s, bytesToTransfer, fp);
    //fprintf(stderr, "AFTER sending initial packets\n");


    int transmit = 0; // 0 --> retransmit packet, 1 --> send new packet
    int curr_ack_type = -1; // 0 --> duplicate ack, 1 --> new ack

    while (LAST_ACK_RECV < bytesToTransfer) {

      // always start off by trying to get any acks
      //fprintf(stderr, "before receive ack\n");
      curr_ack_type = receivePacket(s, bytesToTransfer);
      fprintf(stderr, "DUP_COUNT = %d, curr_ack_type = %d, LAST_ACK_RECV = %llu, STATE = %c\n", DUP_COUNT, curr_ack_type, LAST_ACK_RECV, STATE);
      if (LAST_ACK_RECV >= bytesToTransfer) {fprintf(stderr, "%s\n", "breaking from loop\n");break; }


      if (STATE == 'C') { // timeout should be the first thing to check
        if (checkTimeout()) {
          //fprintf(stderr, "entering timeout state, STATE = %c\n", STATE);
          STATE = 'S';
          SSTHRESH = CWND / 2;
          CWND = MSS;
          DUP_COUNT = 0;
          //fprintf(stderr, "DUP_COUNT SET TO 0, timeout in C <----------------\n");
          transmit = 0;
          TIMEOUT_INTERVAL *= 2;
        }
        else if (DUP_COUNT == 3) {
          //fprintf(stderr, "DUP_COUNT = 3, , STATE = %c <-----------------\n", STATE);
          SSTHRESH = CWND / 2;
          CWND = SSTHRESH + 3 * MSS;
          transmit = 0;
          STATE = 'F';
        }
        else if (curr_ack_type == 1) { // NEW ACK
          CWND = CWND + MSS * (MSS / CWND);
          CWND_START += MSS;
          //fprintf(stderr, "DUP_COUNT SET TO 0, new ack in C <----------------\n");
          DUP_COUNT = 0;
          transmit = 1;
        }

        else if (curr_ack_type == 0) { // DUP ACK
          //fprintf(stderr, "DUP_COUNT INCREMENTING, STATE C <----------------\n");
          DUP_COUNT++;
          transmit = -1;
        }

      }


      else if (STATE == 'F') { // timeout should be the first thing to check
        if (checkTimeout()) {
          //fprintf(stderr, "entering timeout state, STATE = %c <-----------------\n", STATE);
          SSTHRESH = CWND / 2;
          CWND = 1;
          DUP_COUNT = 0;
          transmit = 0;
          STATE = 'S';
          TIMEOUT_INTERVAL *= 2;
        }
        else if (curr_ack_type == 0) { // DUP ACK
          //fprintf(stderr, "entering curr_ack_type = 0 state, STATE = %c<-----------------\n", STATE);
          CWND = CWND + MSS;
          CWND_START += MSS;
          transmit = 1;
        }
        else if (curr_ack_type == 1) { // NEW ACK
          //fprintf(stderr, "entering curr_ack_type = 1 state, STATE = %c<-----------------\n", STATE);
          CWND = SSTHRESH;
          DUP_COUNT = 0;
          STATE = 'C';
          transmit = -1;
        }
      }


      else if (STATE == 'S') { // timeout should be the first thing to check
        if (checkTimeout()) {
          //fprintf(stderr, "entering timeout state, STATE = %c <-----------------\n", STATE);
          SSTHRESH = CWND / 2;
          CWND = MSS;
          //fprintf(stderr, "DUP_COUNT SET TO 0, timeout in S <----------------\n");
          DUP_COUNT = 0;
          transmit = 0;
          TIMEOUT_INTERVAL *= 2;
        }

        else if (DUP_COUNT == 3) {
          //fprintf(stderr, "entering DUP_COUNT = 3 state, STATE = %c<-----------------\n", STATE);
          STATE = 'F';
          SSTHRESH = CWND / 2;
          CWND = SSTHRESH + 3 * MSS;
          transmit = 0;
        }

        else if (CWND >= SSTHRESH) {
          //fprintf(stderr, "entering CWND >= SSTHRESH state, STATE = %c<-----------------\n", STATE);
          STATE = 'C';
          transmit = -1;
        }

        else if (curr_ack_type == 1) { // NEW ACK
          //fprintf(stderr, "entering curr_ack_type = 1 state (dup count set to 0), STATE = %c<-----------------\n", STATE);
          CWND += MSS;
          CWND_START += MSS;
          DUP_COUNT = 0;
          transmit = 1;
        }
        else if (curr_ack_type == 0) { // DUPLICATE ACK <-- new change
          //fprintf(stderr, "entering curr_ack_type = 0 state, STATE = %c<-----------------\n", STATE);
          DUP_COUNT++;
          transmit = -1;
        }
      }

      if (transmit == 1) { // send new packet
        //fprintf(stderr, "entering sendPacket()\n");
        sendPacket(s, bytesToTransfer, fp);
      }

      if (transmit == 0) { // re send a packet
        retransmitPacket(s, bytesToTransfer, filename);
      }

    }

    // send ending packet:
    /*char end_message[256];
    memset(end_message, 0, 256);
    end_message[0] = '-';
    ssize_t end_bytes_sent = sendto(s, end_message, 256, 0, (struct sockaddr*)&si_other, slen);
    if (end_bytes_sent != 256) fprintf(stderr, "failed to send end packet to receiver\n");*/

    while (1) { // continue to send ending packet until you get an ack for it:
      char end_message[1];
      end_message[0] = '-';
      ssize_t bytes_sent = sendto(s, end_message, 1, 0, (struct sockaddr*)&si_other, slen);

      struct timeval timeout_struct; // set timer for 40 ms to send the FIN
      timeout_struct.tv_sec = 0;
      timeout_struct.tv_usec = 40000;
      if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_struct, sizeof(timeout_struct)) < 0)
          perror("setsockopt failed\n");

      char ack_message[1];
      memset(ack_message, 0, 1);
      ssize_t bytes_read = recvfrom(s, ack_message, 1, 0, (struct sockaddr*)&si_other, &slen);
      if (ack_message[0] == '-') {
        break;
      }

    }

    fprintf(stderr, "dammit\n");

    fclose(fp);
    printf("Closing the socket\n");

    close(s);
    return;

}


int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}
