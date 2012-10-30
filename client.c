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

// :: Global Variables :: //
int sockfd= 0;
struct addrinfo *servinfo= NULL;
struct sockaddr_in *servaddr= NULL;
char activeChannel[CHANNEL_MAX];
const char basicChannel[CHANNEL_MAX]= "Common";
std::set<std::string> channelSet;

// :: Function Prototypes :: //
int msg_exit();
int msg_join(const char*);
int msg_leave(const char*);
int msg_list();
int msg_login(const char*);
int msg_switch(const char*);
int msg_who(const char*);
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

	// Set the terminal to raw mode
	//raw_mode();

	memset(activeChannel, '\0', CHANNEL_MAX);

	if (setupSocket(argv[1], argv[2]) != true) {
		//cooked_mode();
		return 1;
	}

	if (msg_login(argv[3]) != true) {
		//cooked_mode();
		return 1;
	}

	int parseStatus= true;
	do {
		char *input= new_inputString();
		parseStatus= parseInput(input);
		free(input);
	} while (parseStatus != -1);

	freeaddrinfo(servinfo);

	//cooked_mode();
	return 0;
}

int handleSwitch(const char *channel) {
	std::set<std::string>::iterator it;
	if ((it= channelSet.find(channel)) != channelSet.end()) {
		strncpy(activeChannel, it->c_str(), CHANNEL_MAX);
		return true;
	}
	return false;
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
	char *line= (char*) malloc(BUFSIZE*sizeof(char));
	int i= 0;
	char c= getchar();
	for (i=0; c != '\n' && i < BUFSIZE; i++, c= getchar()) {
		line[i]= c;
	}

	line[i]= '\0';

	return line;
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
