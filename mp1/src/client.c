/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PORT "80" // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once

char* FILENAME;
char* HOSTNAME;
char* PORTNUM;


ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    size_t total_sent = 0;
    ssize_t result;
    //fprintf(stderr, "%s\n", "CLIENT: writing to server\n");

    while ((total_sent < count) && (result = write(socket, buffer+total_sent, count - total_sent))) {
        //fprintf(stderr, "%s", buffer);
        if (result == -1 && errno == EINTR) {
            continue;
        }
        else if (result == -1) {
            //return -1;
            break;
        }
        else if (result == 0) {
            return 0;
        } else {
            total_sent += result;
        }

    }
    return total_sent;
}


/*  Given second command line input, will initialize HOSTNAME, FILENAME, PORTNUM */
void get_filename(char* domainName) {

	char domainCpy[strlen(domainName) + 1];
	strcpy(domainCpy, domainName);

	int hasPort = 0;
	// traverse domain copy to see if it has > 1 colons in it (meaning optional port):
	char temp[strlen(domainName) + 1];
	strcpy(temp, domainName);

	int count = 0;
	size_t len = strlen(temp);
	for (int i = 0; i < len; i++) {
		if (temp[i] == ':') count++;
	}

	if (count == 2) hasPort = 1;

	char firstColon[1];
	char firstSlash[2];
	char hostname[strlen(domainName) + 1];
	char secondColon[1];
	char port[strlen(domainName) + 1];
	char secondSlash[1];
	char filename[strlen(domainName) + 1];

	if (hasPort) {

		if (7 == sscanf(domainCpy, "%[:htp]%[//]%[a-z.0-9]%1[:]%[0-9]%1[/]%s", firstColon,
			firstSlash, hostname, secondColon, port, secondSlash, filename)) {

      HOSTNAME = strdup(hostname); // need to free
      FILENAME = strdup(filename); // need to free
      PORTNUM = strdup(port);
      fprintf(stderr, "hostname = %s\n", HOSTNAME);
      fprintf(stderr, "filename = %s, strlen(filename) = %zu\n", FILENAME, strlen(FILENAME));
      fprintf(stderr, "port = %s\n", PORTNUM);
		}
	}

  else {
		if (5 == sscanf(domainCpy, "%[:htp]%[//]%[a-z.0-9]%1[/]%s", firstColon,
			firstSlash, hostname, secondColon, filename)) {

    	HOSTNAME = strdup(hostname); // need to free
    	FILENAME = strdup(filename); // need to free
      PORTNUM = PORT;
    }
	}
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count, int* endOfSock) {
     size_t bytes_read = 0;
    while (bytes_read < count) {
      ssize_t red = read(socket, buffer + bytes_read, count - bytes_read);

      if (red == 0 ) {
        fprintf(stderr, "CLIENT read(): reached end of socket --> returning 0\n");
        *endOfSock = 1;
        break;
      }
      if (red == -1 && errno == EINTR) {
        continue;
      }
      if (red == -1) {
        perror("CLIENT read(): errno = ");
        return -1;
      }
      bytes_read += red;
    }
    return bytes_read;
}



int get_server_message(int socket) {
    fprintf(stderr, "entering get_server_message()\n");

    // read message from server 1 by 1 until you see /r/n/r/n
    int firstR = 0;
    int firstN = 0;
    int secR = 0;
    int secN = 0;

    while (1) {
      char* buffer = malloc(2);
      memset(buffer, 0, 1);

      int zero = 0;
      int* endOfSock = &zero;
      size_t read_res = read_all_from_socket(socket, buffer, 1, endOfSock);
      if (read_res == -1) {
        perror("CLIENT read(): errno =");
        break;
      }
      if (*endOfSock) {
        fprintf(stderr, "CLIENT read(): reached end of socket --> returning 0\n");
        break;
      }

      fprintf(stderr, "buffer = %s\n", buffer);
      buffer[1] = '\0';

      if (strcmp(buffer, "\r") == 0 && firstN && firstR) {
        secR = 1;
        fprintf(stderr, "setting second R\n");
      }
      else if (strcmp(buffer, "\r") == 0) {
        firstR = 1;
        fprintf(stderr, "setting first R\n");
      }
      else if (strcmp(buffer, "\n") == 0 && firstR && secR && firstN) {
        secN = 1;
        fprintf(stderr, "setting second N\n");
      }
      else if (strcmp(buffer, "\n") == 0) {
        firstN = 1;
        fprintf(stderr, "setting first N\n");
      }
      else {
        firstR = 0;
        firstN = 0;
        secR = 0;
        secN = 0;
        fprintf(stderr, "resetting\n");
      }

      if ((firstR == 1 && firstN == 1) && (secR == 1 && secN == 1)) return 1;
    }

    return 0;

}

/*  Helper function to send `GET filename HTTP/1.1\r\n\r\n` to server */
void send_get_request(int socket) {

	size_t requestLen = 3 + 1 + strlen(FILENAME) + 1 + 8 + 4;

	char* request = malloc(requestLen);
	memset(request, 0, requestLen);

	size_t sprintfRes = sprintf(request, "GET %s HTTP/1.1\r\n\r\n", FILENAME);
  fprintf(stderr, "requestLen = %zu and sprintf writes %zu bytes\n", requestLen, sprintfRes);
	//fprintf(stderr, "GET request to be sent to server = --%s--\n", request);

  size_t bytes_written = 0;
  size_t min = MAXDATASIZE;

  while (bytes_written < requestLen) {
    if (requestLen - bytes_written < MAXDATASIZE) min = requestLen - bytes_written;

    fprintf(stderr, "GET request to be sent to server = --%s--, min = %zu\n", request, min);
    size_t curr_bytes = write_all_to_socket(socket, request, min);
    if (curr_bytes != min) fprintf(stderr, "failed to write get request to server\n");

    request += curr_bytes;
    bytes_written += curr_bytes;
  }

}


/*  ------------------------------------  */
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*  ------------------------------------------------  */
int main(int argc, char *argv[])
{

	int sockfd;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: ./http_client hostname[:port]/path/to/file\n");
	    exit(1);
	}


	memset(&hints, 0, sizeof hints);
	memset(&servinfo, 0, sizeof servinfo);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// 1. store HOSTNAME and FILENAME from inputted domain
  //char* domainName = calloc(1, strlen(argv[1]));
  //strcpy(domainName, argv[1]);
	get_filename(argv[1]);

	if ((rv = getaddrinfo(HOSTNAME, PORTNUM, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}


	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sockfd == -1) {
			perror("client: socket");
			continue;
		}

		// new addition
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1 && errno != EINPROGRESS) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


	// 2. send GET request to server
  send_get_request(sockfd);

  // prevents client from sending and server from reading anymore
  int shut = shutdown(sockfd, SHUT_WR);
  if (shut != 0) {
      perror("Shutdown failed");
  }

	// 3. wait until server sends response message
  fprintf(stderr, "waiting for response from server..\n");


  size_t res1 = get_server_message(sockfd);

	if (res1) {
		FILE* fptr = fopen("output", "wb");
		if (fptr == NULL) {
			perror("fopen");
      close(sockfd);
			return 1;
		}

    while (1) {
      memset(buf, 0, MAXDATASIZE);

      int zero = 0;
      int* endOfSock = &zero;
      size_t read_res = read_all_from_socket(sockfd, buf, MAXDATASIZE, endOfSock);

      if (read_res == -1) {
        fclose(fptr);
        close(sockfd);
        return 1;
      }
      fprintf(stderr, "writing to output = %s\n", buf);
      size_t written = fwrite(buf, 1, read_res, fptr);
      if (written != read_res) {
        fprintf(stderr, "fwrie() not working\n");
      }

      if (*endOfSock) break;

    }

    fclose(fptr);

	}


	close(sockfd);

	return 0;
}
