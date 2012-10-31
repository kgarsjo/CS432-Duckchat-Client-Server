#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

const char msg[36] = "\0\0\0\0kgarsjo\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char buf[1024];

int main(int argc, char **argv) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	if (argc != 4) {
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_DGRAM;

	if ((rv= getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		printf("failed getaddrinfo\n");
		return 1;
	}

	for(p= servinfo; p != NULL; p= p->ai_next) {
		if ((sockfd= socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			printf("haven't yet found...\n");
			continue;
		}

		if ((numbytes= sendto(sockfd, msg, 36, 0, p->ai_addr, p->ai_addrlen)) == -1) {
			perror("problems sending: ");
			return 3;
		}

		if (p->ai_family == AF_INET) {
			printf("Family:\tINET_4\n");
		} else if (p->ai_family == AF_INET6) {
			printf("Family:\tINET_6\n");
		}
		
		printf("sent %d bytes to host\n", 36);
	}

	if ((numbytes= recvfrom(sockfd, buf, 1024, 0, p->ai_addr, &p->ai_addrlen)) == -1) {
		perror("recvfrom: ");
	}

	printf("Received: %s\n", buf);
	freeaddrinfo(servinfo);

	return 0;
}
