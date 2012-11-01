#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <map>
#include <string>
#include <utility>

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
struct sockaddr_storage lastUser;
size_t lastSize= sizeof(lastUser);

std::map<std::string, std::string> 		map_addrToUser;
std::map<std::string, struct sockaddr_storage>	map_userToAddr;
std::multimap<std::string, std::string>		map_userToChan;
std::multimap<std::string, std::string> 	map_chanToUser;

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
std::string addrToString(const struct sockaddr_in*);
std::string addrToUser(const struct sockaddr_in*);
int addUser(const char*);
int addUserToChannel(const char*, const char*);
void log(FILE*, const char*, const char*, const char*);
void logError(const char*);
void logInfo(const char*);
void logReceived(int, const char*);
void logSent(const char*);
void logWarning(const char*);
char *new_timeStr();
int removeUser(const char*);
int removeUserFromChannel(const char*, const char*);
int sendMessage(const struct sockaddr*, size_t, struct text*, int);
int sendToLast(struct text*, int);
int setupSocket(char*, char*);
int switchRequest(struct request*, int);


// :: PROGRAM ENTRY :: //
int main(int argc, char **argv) {

	if (argc != 2+1){
		printf("%s", "usage: hostname port\n");
		return 1;
	}

	logInfo("Starting Duckchat Server");

	setupSocket(argv[1], argv[2]);
	logInfo("Sockets initialized");
	logInfo("Waiting for requests");	

	int numbytes= 0;
	while (true) {

	// program logic goes here
		struct request *req= (struct request*) malloc(sizeof (struct request) + BUFSIZE); 
		if ((numbytes= recvfrom(sockfd, req, 1024, 0, (struct sockaddr*)&lastUser, &lastSize)) > 0) {
			switchRequest(req, numbytes);
		}
		free(req);
	}

	freeaddrinfo(servinfo);
	return 0;
}

std::string addrToString(const struct sockaddr_in* addr) {
	std::string ipStr= inet_ntoa(addr->sin_addr);
	char port[6];
	snprintf(port, 6, "%hu", addr->sin_port);
	ipStr= (ipStr + ":" + port);
	if (ipStr == "0.0.0.0:0") {
		return "";
	}
	return ipStr;
}

std::string addrToUser(const struct sockaddr_in* addr) {
	std::string addrStr= addrToString(addr);
	return map_addrToUser[addrStr];
}

int addUser(const char* username) {
	if (username == NULL) {
		logError("addUser: username was NULL");
		return false;
	}

	char *format= (char*) malloc(BUFSIZE);
	std::string lastAddr= addrToString((struct sockaddr_in*)&lastUser);
	std::string userStr= username;
	
	struct sockaddr_storage addr= map_userToAddr[userStr];
	std::string storedAddr= addrToString((struct sockaddr_in*)&addr);
	
	if (storedAddr != "" && lastAddr != storedAddr) {
		snprintf(format, BUFSIZE, "Duplucate username %s: replacing record %s with %s",
			username, storedAddr.c_str(), lastAddr.c_str());
		logWarning(format);
		map_addrToUser.erase(storedAddr);
	} else {
		snprintf(format, BUFSIZE, "Logging in user %s with record %s",
			username, lastAddr.c_str());
		logInfo(format);
	}

	map_addrToUser[lastAddr]= userStr;
	map_userToAddr[userStr]= lastUser;
	
	free(format);
	return true;
}

int addUserToChannel(const char *username, const char *channel) {
	if (username == NULL ) {
		logError("addUserToChannel: username was NULL");
		return false;
	} else if (channel == NULL) {
		logError("addUserToChannel: channel was NULL");
		return false;
	}

	std::string userStr= username;
	std::string chanStr= channel;
	map_userToChan.insert( std::pair<std::string,std::string>(userStr, chanStr) );
	map_chanToUser.insert( std::pair<std::string,std::string>(chanStr, userStr) );
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

void logError(const char *msg) {
	log(stdout, " [ERR] ::", msg, ANSI_COLOR_RED);
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
void logReceived(int type, const char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", REQ_STR[type], msg);
	log(stdout, "[RECV] ::", format, ANSI_COLOR_CYAN);
	free(format);
}

void logSent(int type, const char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", TXT_STR[type], msg);
	log(stdout, "[SEND] ::", format, ANSI_COLOR_MAGENTA);
	free(format);	
}

void logWarning(const char* msg) {
	log(stdout, "[WARN] ::", msg, ANSI_COLOR_YELLOW);
}

int msg_error(const char*msg) {
	if (msg == NULL) {
		logError("msg_error: msg was NULL");
		return false;
	}
	struct text_error *txt= (struct text_error*) malloc(sizeof(struct text_error));
	txt->txt_type= TXT_ERROR;
	strncpy(txt->txt_error, msg, SAY_MAX);

	int result= sendToLast((struct text*) txt, sizeof(struct text_error));
	free(txt);
	logSent(TXT_ERROR, msg);
	return result;
}

int msg_list() {

}

int msg_say() {

}

int msg_who() {

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
	logReceived(REQ_KEEP_ALIVE, "");
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
	addUser(req->req_username);
	return true;
}

int recv_logout(struct request_logout *req) {
	logReceived(REQ_LOGOUT, "");
}

int recv_say(struct request_say *req) {
	int size= strlen(req->req_channel) + strlen(req->req_text) + 4;
	char *msg= (char*) malloc(size);
	snprintf(msg, size, "<%s> %s", req->req_channel, req->req_text);
	logReceived(REQ_SAY, msg);
	free(msg);
	return true;
}

int recv_who(struct request_who *req) {
	logReceived(REQ_WHO, req->req_channel);
}

int sendMessage(const struct sockaddr *addr, size_t addrlen, struct text *msg, int msglen) {
	int result= sendto(sockfd, msg, msglen, 0, addr, addrlen);
	if (result == -1) {
		return false;
	}
	return true;
}

int sendToLast(struct text *msg, int msglen) {
	return sendMessage((struct sockaddr*)&lastUser, lastSize, msg, msglen);
}

/*
	setupSocket - Creates the central socket for use by the server.
	Returns: true or false, regarding socket creation success.
*/
int setupSocket(char *addr, char *port) {
	int result= 0;

	// Setup hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_DGRAM;
	hints.ai_flags= AI_PASSIVE;

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

	if ((bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
		perror("bind: ");
		return false;
	}

	return true;
}

int switchRequest(struct request* req, int len) {
	char *warning= (char*) malloc(sizeof(char)*BUFSIZE);
	int result= false;

	if (addrToUser((struct sockaddr_in*)&lastUser) == "" && req->req_type != REQ_LOGIN) {
		snprintf(warning, BUFSIZE, "Received request type %d from unknown user; ignoring", ntohl(req->req_type));
		logWarning(warning);
		free(warning);
		return result;
	}

	switch( ntohl(req->req_type) ) {
	case REQ_LOGIN:
		if (sizeof(struct request_login) != len) {
			snprintf(warning, BUFSIZE, "Received login request; expected %d bytes but was %d bytes", sizeof(struct request_login), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_login( (struct request_login*) req );
		break;
	case REQ_LOGOUT:
		if (sizeof(struct request_logout) != len) {
			snprintf(warning, BUFSIZE, "Received logout request; expected %d bytes but was %d bytes", sizeof(struct request_logout), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_logout( (struct request_logout*) req );
		break;
	case REQ_JOIN:
		if (sizeof(struct request_join) != len) {
			snprintf(warning, BUFSIZE, "Received join request; expected %d bytes but was %d bytes", sizeof(struct request_join), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_join( (struct request_join*) req );
		break;
	case REQ_LEAVE:
		if (sizeof(struct request_leave) != len) {
			snprintf(warning, BUFSIZE, "Received leave request; expected %d bytes but was %d bytes", sizeof(struct request_leave), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_leave( (struct request_leave*) req );
		break;
	case REQ_SAY:
		if (sizeof(struct request_say) != len) {
			snprintf(warning, BUFSIZE, "Received say request; expected %d bytes but was %d bytes", sizeof(struct request_say), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_say( (struct request_say*) req );
		break;
	case REQ_LIST:
		if (sizeof(struct request_list) != len) {
			snprintf(warning, BUFSIZE, "Received list request; expected %d bytes but was %d bytes", sizeof(struct request_list), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_list( (struct request_list*) req );
		break;
	case REQ_WHO:
		if (sizeof(struct request_who) != len) {
			snprintf(warning, BUFSIZE, "Received who request; expected %d bytes but was %d bytes", sizeof(struct request_who), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_who( (struct request_who*) req );
		break;
	case REQ_KEEP_ALIVE:
		if (sizeof(struct request_keep_alive) != len) {
			snprintf(warning, BUFSIZE, "Received keepalive request; expected %d bytes but was %d bytes", sizeof(struct request_keep_alive), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_keepAlive( (struct request_keep_alive*) req );
		break;
	default:
		snprintf(warning, BUFSIZE, "Received unknown request type %d of size %d bytes", req->req_type, len);
		logWarning(warning);
		break;
	}
	free(warning);
	return result;
}
