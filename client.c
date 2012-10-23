#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "client.h"
#include "duckchat.h"

#define true 1
#define false 0
#define BUFSIZE 1024

// :: Global Variables :: //
int sockfd= 0;
struct addrinfo *servinfo= NULL;
struct sockaddr_in *servaddr= NULL;
char msgBuf[BUFSIZE];

int main(int argc, char** argv) {
	// Basic Argument Check
	if (argc != (3+1)) {
		printf("usage: ./client server_socket server_port username\n");
		return 1;
	}

	if (setupSocket(argv[1], argv[2]) != true) {
		return 1;
	}

	if (login(argv[3]) != true) {
		return 1;
	}

	freeaddrinfo(servinfo);

	return 0;
}

int login(char *name) {
	int len= USERNAME_MAX + sizeof(int);
	msgBuf[0]= htonl(REQ_LOGIN);
	snprintf(msgBuf + 1, len - 1, "%s", name);
	return sendMessage(msgBuf, len);
}

int sendMessage(char *msg, int len) {
	int result= sendto(sockfd, msg, len, 0, servinfo->ai_addr, servinfo->ai_addrlen);
	if (result == -1) {
		return false;
	}
	return true;
}

int setupSocket(char *addr, char *port) {
	int result= 0;

	// Setup hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype= SOCK_DGRAM;

	// Fetch address info struct
	if ((result= getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return false;
	}

	// Create UDP socket
	sockfd= socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sockfd == -1) {
		perror("socket: ");
		return false;
	}

	return true;
}
