/*
 * File:   receiver_main.c
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
#include <fcntl.h>

//long long int LAST_SEQ_NUM = 0;
long long int ACK_TO_SEND = 1;
long long int MSS = 1472;
struct timeval timeout_struct;

struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

/* return -1 if sender hasn't sent anything, else return 1, return 0 if it's the last packet */
int receivePacket(int socket, FILE* fp) {
  // will expect to receive packet with sequence number = ACK_TO_SEND
  //fprintf(stderr, "entering receivePacket()\n");

  char expected_seq_num[256];
  ssize_t expected_seq_num_digits = sprintf(expected_seq_num, "%llu", ACK_TO_SEND);
  //fprintf(stderr, "expected_seq_num = --%s--\n", expected_seq_num);
  //fprintf(stderr, "expected_seq digits = %zu\n", expected_seq_num_digits);

  char* recv_message = malloc(expected_seq_num_digits + MSS);
  ssize_t bytes_read = recvfrom(socket, recv_message, expected_seq_num_digits + MSS, 0, (struct sockaddr*)&si_me, &slen);
  timeout_struct.tv_sec = 0;
  timeout_struct.tv_usec = 100;
  if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_struct, sizeof(timeout_struct)) < 0)
      perror("setsockopt failed\n");

  if (bytes_read <= 0) { perror("sender not sending anything rn #1\n"); return -1; }
  if (bytes_read != expected_seq_num_digits + MSS) fprintf(stderr, "failed to receive all bytes of the sequence number\n");


  if (recv_message[0] == '-') {
    // this is the last packet:
    ACK_TO_SEND = -1;
    //fprintf(stderr, "returning 0 receivePacket()\n");
    return 0;
  }

  char recv_seq_num_string[expected_seq_num_digits];
  memcpy(recv_seq_num_string, recv_message, expected_seq_num_digits);
  recv_seq_num_string[expected_seq_num_digits] = 0;
  //fprintf(stderr, "received segment number from sender = --%s--\n", recv_seq_num_string);

  unsigned long long int recv_seq_num = atoll(recv_seq_num_string);
  //fprintf(stderr, "received segment number from sender = %llu\n", recv_seq_num);

  if (recv_seq_num != ACK_TO_SEND) { fprintf(stderr, "returning 1 receivePacket()\n");return 1;} // send ack again with same ACK_TO_SEND

  recv_message += expected_seq_num_digits;
  ssize_t app_message_len = bytes_read - expected_seq_num_digits;

  char* app_message = malloc(app_message_len);
  memcpy(app_message, recv_message, app_message_len);
  app_message[app_message_len] = 0;
  //fprintf(stderr, "received app_message = --%s--\n", app_message);

  // write into file
  size_t bytes_written = 0;
  while (bytes_written < app_message_len) {

    size_t min = MSS;
    if (app_message_len - bytes_written < min) min = app_message_len - bytes_written;

    ssize_t curr_bytes_written = fwrite(app_message, 1, min, fp);
    if (curr_bytes_written != min) fprintf(stderr, "fwrite didn't write everything to the file\n");

    app_message += curr_bytes_written;
    bytes_written += curr_bytes_written;
  }

  ACK_TO_SEND = recv_seq_num + app_message_len;
  //fprintf(stderr, "returning, setting ACK_TO_SEND = %llu\n", ACK_TO_SEND);
  return 1;
}


void sendAck(int socket) {

  //fprintf(stderr, "RECEIVER: ENTERING sendAck()\n");

  if (ACK_TO_SEND == -1) {
    char ack_string[256];
    ack_string[0] = '-';
    ack_string[1] = 0;
    //fprintf(stderr, "sening back fin\n");
    ssize_t bytes_sent = sendto(socket, ack_string, 1, 0, (struct sockaddr*)&si_me, slen);
    if (bytes_sent <= 0) {fprintf(stderr, "failed to send ack\n");}
  }
  else {
    char ack_string[256];
    ssize_t num_digits = sprintf(ack_string, "%llu", ACK_TO_SEND);

    //fprintf(stderr, "sending this to sender = --%s--\n", ack_string);
    ssize_t bytes_sent = sendto(socket, ack_string, num_digits, 0, (struct sockaddr*)&si_me, slen);
    if (bytes_sent <= 0) {fprintf(stderr, "failed to send ack\n");}
  }

}


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding, port = %d\n", myUDPport);
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    //fcntl(s, F_SETFL, O_NONBLOCK);

	/* Now receive data and send acknowledgements */

  FILE* fp;
  fp = fopen(destinationFile, "wb");
  if (fp == NULL) {
    printf("Could not open file to send.");
    exit(1);
  }

  // create receive buffer
  while (1) {
    int received_pkt = receivePacket(s, fp);

    //if (received_pkt == 0) break;
    //if (received_pkt != -1) sendAck(s);
    sendAck(s);
    if (ACK_TO_SEND == -1) break;
  }

  fclose(fp);

  close(s);
	printf("%s received.", destinationFile);
  return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
