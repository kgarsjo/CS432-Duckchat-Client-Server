#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define true 1
#define false 0
#define BUFSIZE 1024
#define SRV_PORT "2222"

#define ANSI_COLOR_RED 		"\x1b[31m"
#define ANSI_COLOR_GREEN 	"\x1b[32m"
#define ANSI_COLOR_YELLOW 	"\x1b[33m"
#define ANSI_COLOR_BLUE		"\x1b[34m"
#define ANSI_COLOR_MAGENTA	"\x1b[35m"
#define ANSI_COLOR_CYAN		"\x1b[36m"
#define ANSI_COLOR_RESET	"\x1b[0m"

int sockfd= 0;
struct addrinfo *servinfo= NULL;
char buf[1024];

void log(FILE*, char*, char*);
void logReceived(int, char*);
void logSent(char*);
int setupSocket();

int main() {

	setupSocket();
	
	int numbytes= 0;
	while (true) {
		if ((numbytes= recvfrom(sockfd, buf, 1024-1, 0, servinfo->ai_addr, &servinfo->ai_addrlen)) > 0) {
			buf[numbytes]= '\0';			
			logReceived(buf[0], buf + 1);
		}
	}

	return 0;
}

void log(FILE *out, char *header, char *msg, char *color=ANSI_COLOR_RESET) {
	fprintf(out, "%s%s %s%s\n", color, header, msg, ANSI_COLOR_RESET);
}

void logReceived(int type, char *msg) {
	snprintf(msg, BUFSIZE, "req=%d, msg=\"%s\"", type, msg);
	log(stdout, "[RECV] ::", msg, ANSI_COLOR_CYAN);
}

int setupSocket() {
	int result= 0;

	// Setup hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_UNSPEC;
	hints.ai_socktype= SOCK_DGRAM;
	hints.ai_flags= AI_PASSIVE;

	// Fetch address info struct
	if ((result= getaddrinfo(NULL, SRV_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return false;
	}

	// Create UDP socket
	sockfd= socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sockfd == -1) {
		perror("socket: ");
		return false;
	}

	if ((bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
		perror("bind: ");
		return false;
	}

	return true;
}
