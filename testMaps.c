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

#include <map>
#include <string>

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
struct sockaddr_storage *stringToAddr(std::string, size_t*);
std::string addrToUser(const struct sockaddr_in*);



// :: PROGRAM ENTRY :: //
int main(int argc, char **argv) {
	struct addrinfo *servinfo;
	struct sockaddr *p;
	
	getaddrinfo("localhost", "2345", NULL, &servinfo);
	std::string addrStr= addrToString((struct sockaddr_in*)servinfo->ai_addr);
	printf("toString: %s\n", addrStr.c_str());
	size_t size= 0;	
	struct sockaddr_storage *newAddr= stringToAddr(addrStr, &size);
}

std::string addrToString(const struct sockaddr_in* addr) {
	std::string ipStr= inet_ntoa(addr->sin_addr);
	char port[6];
	snprintf(port, 6, "%hu", addr->sin_port);
	return (ipStr + ":" + port);
}

std::string addrToUser(const struct sockaddr_in* addr) {
	std::string addrStr= addrToString(addr);
	return map_addrToUser[addrStr];
}

struct sockaddr_storage *stringToAddr(std::string addrStr, size_t *addrlen) {
	char *str= (char*) malloc(1000);
	strcpy(str, addrStr.c_str());
	char *ipStr= strtok(str, ":");
	char *portStr= strtok(NULL, ":");

	struct sockaddr_in *saddr= (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	inet_aton(ipStr, &saddr->sin_addr);
	saddr->sin_port= (u_short) atoi(portStr);
	*addrlen= sizeof(saddr);
	return (struct sockaddr_storage*) saddr;
}
