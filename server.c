#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include "duckchat.h"
#include "server.h"

#define true 1
#define false 0
#define BUFSIZE 1024
#define TIMESIZE 100
#define SRV_PORT "2222"

#define ANSI_COLOR_RED 		"\x1b[31m"
#define ANSI_COLOR_GREEN 	"\x1b[32m"
#define ANSI_COLOR_YELLOW 	"\x1b[33m"
#define ANSI_COLOR_BLUE		"\x1b[34m"
#define ANSI_COLOR_MAGENTA	"\x1b[35m"
#define ANSI_COLOR_CYAN		"\x1b[36m"
#define ANSI_COLOR_RESET	"\x1b[0m"

// :: GLOBAL VALUES :: //
int sockfd= 0;
struct addrinfo *servinfo= NULL;
struct user_profile *users= NULL;
char buf[1024];


// :: CONSTANT VALUES :: //
const char* const REQ_STR[8] = {"REQ_LOGIN",
								"REQ_LOGOUT",
								"REQ_JOIN",
								"REQ_LEAVE",
								"REQ_SAY",
								"REQ_LIST",
								"REQ_WHO",
								"REQ_KEEP_ALIVE"
								};
const char* const TXT_STR[4] = {"TXT_SAY",
								"TXT_LIST",
								"TXT_WHO",
								"TXT_ERROR"
								}; 



// :: FUNCTION PROTOTYPES :: //
void log(FILE*, const char*, const char*, const char*);
void logInfo(const char*);
void logReceived(int, char*);
void logSent(char*);
char *new_timeStr();
int setupSocket();
int switchRequest(struct request*);


// :: PROGRAM ENTRY :: //
int main() {
	logInfo("Starting Duckchat Server");

	setupSocket();
	logInfo("Sockets initialized");
	logInfo("Waiting for requests");	

	int numbytes= 0;
	while (true) {

	// program logic goes here
		struct request *req= (struct request*) malloc(sizeof (struct request) + BUFSIZE); 
		if ((numbytes= recvfrom(sockfd, req, 1024-1, 0, servinfo->ai_addr, &servinfo->ai_addrlen)) > 0) {
			switchRequest(req);
		}
		free(req);
	}

	freeaddrinfo(servinfo);

	return 0;
}


int addUser(struct user_profile *user) {
	if (users == NULL) {
		users= user;
		return true;
	}

	struct user_profile *current= users;
	do {
		if ( strcmp(current->username, user->username) == 0 ) {
			return false;
		}
	} while (current->next != NULL);

	return true;
}


struct request *getRequest() {
	struct sockaddr src_addr;
	socklen_t addrlen;
	int numbytes= 0;

	return NULL;
}


/*
	log - Logs the given output to the specified file stream, with formatting.
		out:		The file stream to log to
		header:		The log message header to print
		msg:		The message to log
		[color]:	The optional ANSI color to use
*/
void log(FILE *out, const char *header, const char *msg, const char *color=ANSI_COLOR_RESET) {
	char *timeStr= new_timeStr();
	fprintf(out, "%s%s [%s] %s%s\n", color, header, timeStr, msg, ANSI_COLOR_RESET);
	free(timeStr);
}


/*
	logInfo - Logs the given informational message.
		msg:		The message to log
*/
void logInfo(const char *msg) {
	log(stdout, "[INFO] ::", msg, ANSI_COLOR_GREEN);
}


/*
	logReceived - Logs a message received from the clients.
		type:		The request type
		msg:		The remainder of the request
*/
void logReceived(int type, char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", REQ_STR[type], msg);
	log(stdout, "[RECV] ::", format, ANSI_COLOR_CYAN);
	free(format);
}


/*
	new_timeStr - Creates a new string giving the time of day.
	Returns: A new heap-allocated char*.

	NOTE: The caller is obligated to free this returned value.
*/
char *new_timeStr() {
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo= localtime(&rawtime);

	char *timeStr= (char*) malloc(TIMESIZE * sizeof(char));
	strftime(timeStr, TIMESIZE, "%X %x", timeinfo);
	return timeStr;
}

int recv_join(struct request_join *req) {
	logReceived(REQ_JOIN, req->req_channel);
	return true;
}

int recv_keepAlive(struct request_keep_alive *req) {
	return true;
}

int recv_leave(struct request_leave *req) {
	logReceived(REQ_LEAVE, req->req_channel);
	return true;
}

int recv_list(struct request_list *req) {
	logReceived(REQ_LIST, "");
	return true;
}

int recv_login(struct request_login *req) {
	logReceived(REQ_LOGIN, req->req_username);
	return true;
}

int recv_logout(struct request_logout *req) {
	logReceived(REQ_LOGOUT, "");
}

int recv_say(struct request_say *req) {
	int size= strlen(req->req_channel) + strlen(req->req_text) + 3;
	char *msg= (char*) malloc(size);
	snprintf(msg, size, "<%s> %s", req->req_channel, req->req_text);
	logReceived(REQ_SAY, msg);
	free(msg);
	return true;
}

int recv_who(struct request_who *req) {
	logReceived(REQ_WHO, req->req_channel);
}

int removeUser(struct sockaddr src_addr) {
	return false;
}


/*
	setupSocket - Creates the central socket for use by the server.
	Returns: true or false, regarding socket creation success.
*/
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

int switchRequest(struct request* req) {
	switch( htonl(req->req_type) ) {
	case REQ_LOGIN:
		return recv_login( (struct request_login*) req );
	case REQ_LOGOUT:
		return recv_logout( (struct request_logout*) req );
	case REQ_JOIN:
		return recv_join( (struct request_join*) req );
	case REQ_LEAVE:
		return recv_leave( (struct request_leave*) req );
	case REQ_SAY:
		return recv_say( (struct request_say*) req );
	case REQ_LIST:
		return recv_list( (struct request_list*) req );
	case REQ_WHO:
		return recv_who( (struct request_who*) req );
	case REQ_KEEP_ALIVE:
		return recv_keepAlive( (struct request_keep_alive*) req );
	default:
		return false;
	}
}
