#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <set>

#include "duckchat.h"
#include "raw.h"

#define true 1
#define false 0
#define BUFSIZE 1024
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m" 

// :: Global Variables :: //
int sockfd= 0;
struct addrinfo *servinfo= NULL;
struct sockaddr_in *servaddr= NULL;
char activeChannel[CHANNEL_MAX];
const char basicChannel[CHANNEL_MAX]= "Common";
char inBuffer[SAY_MAX + 1];
char *bufPosition= inBuffer;
std::set<std::string> channelSet;

// :: Function Prototypes :: //
void deprompt();
int msg_exit();
int msg_join(const char*);
int msg_leave(const char*);
int msg_list();
int msg_login(const char*);
int msg_switch(const char*);
int msg_who(const char*);
char *new_inputString();
int parseInput(char*);
void prompt();
int sendMessage(struct request*, int);
int setupSocket(char*, char*);
int switchResponse(struct text*);

int main(int argc, char** argv) {
	// Basic Argument Check
	if (argc != (3+1)) {
		printf("usage: ./client server_socket server_port username\n");
		return 1;
	}

	// Set the terminal to raw mode
	raw_mode();

	memset(activeChannel, '\0', CHANNEL_MAX);

	if (setupSocket(argv[1], argv[2]) != true) {
		cooked_mode();
		return 1;
	}

	if (msg_login(argv[3]) != true) {
		cooked_mode();
		return 1;
	}

	int parseStatus= true;
	char *input;
	fd_set readfds;
	prompt();

	do {
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(sockfd, &readfds);
		select(sockfd+1, &readfds, NULL, NULL, NULL);
		
		if (FD_ISSET(0, &readfds)) {
			input= new_inputString();
			if (input != NULL) {
				parseStatus= parseInput(input);
				if (parseStatus != -1) {prompt();}
			}
		} else if (FD_ISSET(sockfd, &readfds)) {
			struct text *txt= (struct text*) malloc(sizeof(struct text) + 1024);
			int numbytes= 0;
			if ((numbytes= recvfrom(sockfd, txt, 1024, 0, servinfo->ai_addr, &servinfo->ai_addrlen)) > 0) {
				deprompt();
				switchResponse(txt);
				free(txt);
				prompt();
			}
		}
	} while (parseStatus != -1);

	freeaddrinfo(servinfo);

	cooked_mode();
	return 0;
}

void deprompt() {
	printf("\b\b");
	fflush(stdout);
}

int handleSwitch(const char *channel) {
	std::set<std::string>::iterator it;
	if ((it= channelSet.find(channel)) != channelSet.end()) {
		strncpy(activeChannel, it->c_str(), CHANNEL_MAX);
		return true;
	}
	return false;
}

int logError(const char *msg) {
	fprintf(stderr, "%s[ ERR] :: %s%s\n", ANSI_COLOR_RED, msg, ANSI_COLOR_RESET);
	fflush(stderr);
	return true;
}

int msg_exit() {
	struct request_logout *req= (struct request_logout*) malloc(sizeof(struct request_logout));
	req->req_type= htonl(REQ_LOGOUT);

	sendMessage((struct request*)req, sizeof(struct request_logout));
	free(req);
	return -1;
}

int msg_join(const char *channel) {
	if (channel == NULL) {
		logError("usage: /join <channel>");
		return false;
	}
	struct request_join *req= (struct request_join*) malloc(sizeof(struct request_join));
	req->req_type= htonl(REQ_JOIN);
	strncpy(req->req_channel, channel, CHANNEL_MAX);
	
	int result= sendMessage( (struct request*) req, sizeof(struct request_join));
	if (result == true) {
		std::string str= channel;
		channelSet.insert(str);
	}
	strncpy(activeChannel, channel, CHANNEL_MAX);
	free(req);
	return result;
}

int msg_leave(const char *channel) {
	if (channel == NULL) {
		logError("usage: /leave <channel>");
		return false;
	}
	struct request_leave *req = (struct request_leave*) malloc(sizeof(struct request_leave));
	req->req_type= htonl(REQ_LEAVE);
	strncpy(req->req_channel, channel, CHANNEL_MAX);

	int result= sendMessage( (struct request*) req, sizeof(struct request_leave));
	if (result == true) {
		std::string str= channel;
		channelSet.erase(str);
	}
	free(req);
	return result;
}

int msg_list() {
	struct request_list *req= (struct request_list*) malloc(sizeof(struct request_list));
	req->req_type= htonl(REQ_LIST);

	int result= sendMessage( (struct request*) req, sizeof(struct request_leave));
	free(req);
	return result;
}

int msg_login(const char *name) {
	struct request_login *req= (struct request_login*) malloc(sizeof(struct request_login));
	req->req_type= htonl(REQ_LOGIN);
	strncpy(req->req_username, name, USERNAME_MAX);

	int result= sendMessage( (struct request*) req, sizeof(struct request_login));
	free(req);
	return result && msg_join(basicChannel);
}

int msg_say(const char *msg) {
	if (msg == NULL || strcmp(activeChannel, "") == 0) {
		return false;
	}
	struct request_say *req= (struct request_say*) malloc(sizeof(struct request_say));
	req->req_type= htonl(REQ_SAY);
	strncpy(req->req_channel, activeChannel, CHANNEL_MAX);
	strncpy(req->req_text, msg, SAY_MAX);
	
	int result= sendMessage( (struct request*) req, sizeof(struct request_say));
	free(req);
	return result;
}

int msg_who(const char *channel) {
	if (channel == NULL) {
		logError("usage: /who <channel>");
		return false;
	}
	struct request_who *req= (struct request_who*) malloc(sizeof(struct request_who));
	req->req_type= htonl(REQ_WHO);
	strncpy(req->req_channel, channel, CHANNEL_MAX);

	int result= sendMessage( (struct request*) req, sizeof(struct request_who));
	free(req);
	return result;
}

char *new_inputString() {
	char c= getchar();
	if (c == '\n') {
		*bufPosition++= '\0';
		bufPosition= inBuffer;
		printf("\n");
		fflush(stdout);
		return inBuffer;
	} else if (((int)c) == 127 && bufPosition > inBuffer) { // Check for backspace
		--bufPosition;
		printf("\b");
		fflush(stdout);
	} else if (bufPosition != inBuffer + SAY_MAX) {
		*bufPosition++ = c;
		printf("%c", c);
		fflush(stdout);
		return NULL;
	}
	return NULL;
}

int parseInput(char *input) {
	const char *DELIM= " \n";
	char *tok= (char*) malloc(sizeof(char)*(strlen(input) + 1));
	strcpy(tok, input);
	char *command= strtok(tok, DELIM);
	if (command == NULL) {
		printf("You shouldn't see this shit");
		return -1;
	}

	int result= false;
	if (0 == strcmp(command, "/exit")) {
		result= msg_exit();
	} else if (0 == strcmp(command, "/join")) {
		char *channel= strtok(NULL, DELIM);
		result= msg_join(channel);
	} else if (0 == strcmp(command, "/leave")) {
		char *channel= strtok(NULL, DELIM);
		result= msg_leave(channel);
	} else if (0 == strcmp(command, "/list")) {
		result= msg_list();
	} else if (0 == strcmp(command, "/who")) {
		char *channel= strtok(NULL, DELIM);
		result= msg_who(channel);
	} else if (0 == strcmp(command, "/switch")) {
		char *channel= strtok(NULL, DELIM);
		result= handleSwitch(channel);
	} else { /* /say */ 
		result= msg_say(input);
	}

	free(tok);
	return result;
}

void prompt() {
	printf("> ");
	fflush(stdout);
}

int recv_error(struct text_error *err) {
	return logError(err->txt_error);	
}

int recv_list(struct text_list *list) {
	
}

int recv_say(struct text_say *say) {
	printf("[%s] [%s] %s\n", say->txt_channel, say->txt_username, say->txt_text);
	fflush(stdout);
	return true;
}

int recv_who(struct text_who *who) {

}

int sendMessage(struct request *req, int len) {
	int result= sendto(sockfd, req, len, 0, servinfo->ai_addr, servinfo->ai_addrlen);
	if (result == -1) {
		return false;
	}
	return true;
}

int setupSocket(char *addr, char *port) {
	int result= 0;

	// Setup hints
	struct addrinfo hints;
	struct addrinfo *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_DGRAM;

	// Fetch address info struct
	if ((result= getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return false;
	}

	// Create UDP socket
	for (p= servinfo; p != NULL; p= p->ai_next) {
		sockfd= socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			perror("socket: ");
			continue;
		}
		break;
	}
	servinfo= p;

	return (p != NULL);
}

int switchResponse(struct text *text) {
	switch (text->txt_type) {
	case TXT_ERROR:
		return recv_error((struct text_error*) text);
	case TXT_LIST:
		return recv_list((struct text_list*) text);
	case TXT_SAY:
		return recv_say((struct text_say*) text);
	case TXT_WHO:
		return recv_who((struct text_who*) text);
	default:
		logError("Unknown response type received");
		return false;
	}
}
