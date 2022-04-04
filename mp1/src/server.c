/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(/*int s*/)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

size_t write_all_to_socket(int socket, char *request, size_t count) {
	size_t total_sent = 0;
	ssize_t result;

	//FILE* sockfd = fdopen(socket, "wb");
	//fprintf(stderr, "%s\n", "SERVER: writing to client\n");
	fprintf(stderr, "SERVER: writing to client\n");

	while ((total_sent < count) && (result = send(socket, request+total_sent, count - total_sent, 0)) /*(result = fwrite(request + total_sent, 1, count - total_sent, sockfd))*/) {
		fprintf(stderr, "%s", "SERVER: writing to client1\n");
    if (result == -1 && errno == EINTR) {
        continue;
    }
    else if (result == -1) {
				break;
    }
    else if (result == 0) {
				return 0;
    }
		else {
		total_sent += result;
	}

  }
	//fprintf(stderr, "SERVER: returning from write, buffer = %s\n", request);
	return total_sent;
}


size_t read_all_from_socket(int socket, char *buffer, size_t count) {
	//fprintf(stderr, "SERVER: entering read_all_from_socket(), buffer = --%s--\n", buffer);
	size_t bytes_read = 0;

    while (bytes_read < count) {
      ssize_t red = read(socket, buffer + bytes_read, count - bytes_read);
			fprintf(stderr, "SERVER1: entering read_all_from_socket(), buffer = --%s--, red = %zu, bytes_read = %zu\n", buffer, red, bytes_read);

			if (red == -1 && errno == EINTR) {
				perror("SERVER read() continuiing in loop: errno = ");
        continue;
      }
      if (red == -1) {
				perror("SERVER read(): errno = ");
        return -1;
      }

      if (red == 0 ) {
				fprintf(stderr, "SERVER read(): reached end of socket --> returning 0\n");
        break;
      }

			char bufferCopy[bytes_read + red];
			memcpy(bufferCopy, buffer, bytes_read + red);
			fprintf(stderr, "this is bufferCopy = --%s--\n", bufferCopy);

			char* newline = strstr(bufferCopy, "\r\n\r\n");

			if (newline != NULL) {
				int indexOfNewline = newline - bufferCopy;

				buffer[indexOfNewline + 4] = '\0';
				fprintf(stderr, "found the two newlines, this is buffer now = --%s--\n", buffer);
				bytes_read += red;
				break;
			}

      bytes_read += red;
    }
    return bytes_read;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	char* portNum = PORT;
	if (argc != 2) {
	    fprintf(stderr,"usage: ./http_server port\n");
	    exit(1);
	} else {
		portNum = argv[1];
	}

	if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...%s\n", portNum);

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}



		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			// 1. Read GET request from client
			char* getRequest = malloc(1024);
			//memset(getRequest, 0, 1024);

			size_t bytes_read = read_all_from_socket(new_fd, getRequest, 1024);
			getRequest[bytes_read] = '\0';
			fprintf(stderr, "bytes read from client = %zu and strlen(getRequest) = %zu\n", bytes_read, strlen(getRequest));

			if (bytes_read <= 0) {
				fprintf(stderr, "server failed to read get request from client, getRequest = %s\n", getRequest);

				close(new_fd);
				exit(1);
			}

			char start[1024];
			strncpy(start, getRequest, 4);
			start[4] = 0;

			char* end = getRequest + (strlen(getRequest) - 4);
			fprintf(stderr, "strlen(end) = %zu\n", strlen(end));
			fprintf(stderr, "end = --%s--\n", end);
			fprintf(stderr, "start = --%s--\n", start);
			fprintf(stderr, "getRequest = --%s--\n", getRequest);

			if (strcmp(start, "GET ") != 0 || strcmp(end, "\r\n\r\n") != 0) {
				write_all_to_socket(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 28);
				fprintf(stderr, "sending to client 400 Bad Request\n");

				close(new_fd);
				exit(1);
			}

			// 2. parse GET request
			fprintf(stderr, "GET request = --%s--\n", getRequest);
			char* httpSubstr = strstr(getRequest, " HTTP/1");
			int indexOfHttp = httpSubstr - getRequest;
			//fprintf(stderr, "httpSubstr = %s, index of http = %d\n", httpSubstr, indexOfHttp);

			char* filename;
			if (getRequest[4] == '/') {
				filename = malloc(indexOfHttp - 5); // getting rid of the 'GET /'
				memset(filename, 0, indexOfHttp - 5);

				//memcpy(filename, &getRequest[5], indexOfHttp - 5);
				strncpy(filename, getRequest + 5, indexOfHttp - 5);
				filename[(indexOfHttp - 5)] = '\0';
			}
			else {
				filename = malloc(indexOfHttp - 4); // getting rid of the 'GET '
				memset(filename, 0, indexOfHttp - 4);

				//memcpy(filename, &getRequest[4], indexOfHttp - 4);
				strncpy(filename, getRequest + 4, indexOfHttp - 4);
				//fprintf(stderr, "getRequest + 4 = %s, indexOfHttp - 4 = %d\n", getRequest + 4, indexOfHttp - 4);
				filename[(indexOfHttp - 4)] = '\0';
			}
			fprintf(stderr, "filename parsed from GET request = --%s--\n", filename);


			// 3. Write back the status to client
			FILE* fp = fopen(filename, "rb");

			if (fp == NULL) {

				write_all_to_socket(new_fd, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
				fprintf(stderr, "sending 404 Not Found\n");

				close(new_fd);
				exit(1);
			}

			//fcntl(new_fd, F_SETFL, O_NONBLOCK); // new addition

			// 4. Get contents of the file
			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			rewind(fp);

			fprintf(stderr, "SERVER: the file size is %zu\n", fileSize);

			char* successMessage = malloc(1024);
			int sprintfRes = sprintf(successMessage, "HTTP/1.1 200 OK %zu\r\n\r\n", fileSize);
			fprintf(stderr, "bytes written into success message = %d\n", sprintfRes);

			//write_all_to_socket(new_fd, successMessage, (size_t)sprintfRes);
			write_all_to_socket(new_fd, successMessage, (size_t)sprintfRes);
			//fprintf(stderr, "sending success message --%s--\n", successMessage);

			size_t bytesWritten = 0;
	    size_t left = fileSize;
	    size_t min = 1024;
	    char* buffer;
			//fprintf(stderr, "SERVER: fileSize = %zu\n", fileSize);

			while (left > 0) {
				//fprintf(stderr, "SERVER: entering while loop, bytesWritten = %zu, left = %zu\n", bytesWritten, left);
				fprintf(stderr, "infinite while loop bytesWritten = %zu, left = %zu\n", bytesWritten, left);
        if (left < 1024) {
            min = left;
        } else {
            min = 1024;
        }

				//char buffer[min];
				//memset(buffer, 0, min);
				buffer = calloc(1, min);

				//fprintf(stderr, "SERVER: entering while loop, min = %zu\n", min);
				fprintf(stderr, "%s\n", "before fread()\n");
				size_t readResult = fread(buffer, 1, min, fp);
				if (min != readResult) {
					fprintf(stderr, "NOOOO server fread() gone wrong\n" );
					//exit(1);
				}

				fprintf(stderr, "before write_all_to_socket(), readResult = %zu, sizeof(buffer) = %zu\n", readResult, sizeof(buffer));
				size_t res = write_all_to_socket(new_fd, buffer, readResult);
				fprintf(stderr, "%s\n", "after write_all_to_socket()\n");
				if (res != readResult) {
					fprintf(stderr, "NOOOO server write_all_to_socket() gone wrong\n");
					//exit(1);
				}

				bytesWritten += res; // changed it from min
        left -= res; // changed it from min
			}
			fprintf(stderr, "exiting loop, bytesWritten = %zu\n", bytesWritten);

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
