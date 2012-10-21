#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "logging.h"

#define true 1
#define false 0

int sockfd=0;
char buf[1024];
struct addrinfo *servinfo;

int main(int argc, char** argv) {
	// Basic Argument Check
	if (argc != (3+1)) {
		printf("usage: ./client server_socket server_port username\n");
		return 1;
	}
	
	char *addr= argv[1];
	char *port= argv[2];
	char *name= argv[3];

	// Setup hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype= SOCK_DGRAM;
	
	// Prep socket for use
	int result= 0;
	struct addrinfo *servinfo;
	if ((result= getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
	}

	// Create socket
	sockfd= socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (result == -1) {
		perror("socket create: ");
	}

	// Begin main loop
	while (true) {
	//	sendto(sockfd, "test\0", 5, 0); 
	}

	return 0;
}
