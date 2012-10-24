#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "duckchat.h"

#define true 1
#define false 0
#define BUFSIZE 1024

// :: Global Variables :: //
int sockfd= 0;
struct addrinfo *servinfo= NULL;
struct sockaddr_in *servaddr= NULL;
char msgBuf[BUFSIZE];


// :: Function Prototypes :: //
int msg_exit();
int msg_join(char*);
int msg_leave(char*);
int msg_list();
int msg_login(char*);
int msg_switch(char*);
int msg_who(char*);
char *new_inputString();
int parseInput(char*);
int sendMessage(struct request*, int);
int setupSocket(char*, char*);


int main(int argc, char** argv) {
	// Basic Argument Check
	if (argc != (3+1)) {
		printf("usage: ./client server_socket server_port username\n");
		return 1;
	}

	if (setupSocket(argv[1], argv[2]) != true) {
		return 1;
	}

	if (msg_login(argv[3]) != true) {
		return 1;
	}

	while (true) {
		char *input= new_inputString();
		parseInput(input);
	}
	freeaddrinfo(servinfo);

	return 0;
}

int msg_exit() {
	return false;
}

int msg_join(char *channel) {
	return false;
}

int msg_leave(char *channel) {
	return false;
}

int msg_list() {
	return false;
}

int msg_login(char *name) {
	struct request_login *req= (request_login*) malloc(sizeof(request_login));
	req->req_type= htonl(REQ_LOGIN);
	strncpy(req->req_username, name, USERNAME_MAX - 1);
	req->req_username[USERNAME_MAX]= '\0';

	return sendMessage( (struct request*)req, sizeof(request_login));
}

int msg_switch(char *channel) {
	return false;
}

int msg_who(char *channel) {
	return false;
}

char *new_inputString() {
	return NULL;
}

int parseInput(char *input) {
	return false;
}

int sendMessage(struct request *req, int len) {
	char *reqbuf= (char*) malloc(len * sizeof(char));
	memcpy(reqbuf, req, len);

	int result= sendto(sockfd, reqbuf, len, 0, servinfo->ai_addr, servinfo->ai_addrlen);
	free(reqbuf);
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
