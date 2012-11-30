#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "duckchat.h"

#define true 1
#define false 0
#define BUFSIZE 1024
#define TIMESIZE 100

#define ANSI_COLOR_RED 		"\x1b[31m"
#define ANSI_COLOR_GREEN 	"\x1b[32m"
#define ANSI_COLOR_YELLOW 	"\x1b[33m"
#define ANSI_COLOR_BLUE		"\x1b[34m"
#define ANSI_COLOR_MAGENTA	"\x1b[35m"
#define ANSI_COLOR_CYAN		"\x1b[36m"
#define ANSI_COLOR_RESET	"\x1b[0m"

// :: GLOBAL VALUES :: //
int sockfd= 0;
int maxfd= 0;
struct addrinfo *servinfo= NULL;
struct sockaddr_storage lastUser;
size_t lastSize= sizeof(lastUser);

std::set<std::string>						set_chanList;
std::set<long long>							set_sayList;
std::vector<std::string>					vec_serverAddrs;
std::map<std::string, std::string> 			map_addrToUser;
std::map<std::string, std::string>			map_userToAddr;
std::multimap<std::string, std::string>		map_userToChan;
std::multimap<std::string, std::string> 	map_chanToUser;

char buf[1024];


// :: CONSTANT VALUES :: //
const char* const REQ_STR[11] = {"REQ_LOGIN",
								"REQ_LOGOUT",
								"REQ_JOIN",
								"REQ_LEAVE",
								"REQ_SAY",
								"REQ_LIST",
								"REQ_WHO",
								"REQ_KEEP_ALIVE",
								"S2S_JOIN",
								"S2S_LEAVE",
								"S2S_SAY"
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
long long genuid();
void log(FILE*, const char*, const char*, const char*);
void logError(const char*);
void logInfo(const char*);
void logReceived(int, const char*);
void logSent(const char*);
void logWarning(const char*);
int msg_error(const char*);
int msg_list();
int msg_s2s_join(const char*);
int msg_s2s_leave(char*);
int msg_s2s_say(char*, char*, char*);
int msg_say(const char*, const char*, const char*, const struct sockaddr*);
int msg_who(const char*);
struct sockaddr *new_stringToAddr(std::string);
char *new_timeStr();
int recv_join(struct request_join*);
int recv_keepAlive(struct request_keep_alive*);
int recv_leave(struct request_leave*);
int recv_list(struct request_list*);
int recv_login(struct request_login*);
int recv_logout(struct request_logout*);
int recv_say(struct request_say*i, long long, char*);
int recv_who(struct request_who*);
int removeLastUser();
int removeUserFromChannel(const char*, const char*);
int s2s_broadcast(struct request*, int);
int s2s_forward(struct request*, int);
int s2s_send(const struct sockaddr*, size_t, struct request*, int);
int s2s_sendToLast(struct request*, int);
int sendMessage(const struct sockaddr*, size_t, struct text*, int);
int sendToLast(struct text*, int);
std::string setupConnection(char*, char*);
int setupSocket(char*, char*);
int switchRequest(struct request*, int);


// :: PROGRAM ENTRY :: //
int main(int argc, char **argv) {

	// Argcheck, count must not be even, must be > 1
	if (argc % 2 == 0 || argc <= 1){
		printf("%s", "usage: hostname port <neighborhost neighborport>*\n");
		return 1;
	}

	logInfo("Starting Duckchat Server");

	int i;
	char *format= (char*) malloc(sizeof(char) * BUFSIZE);
	
	setupSocket(argv[1], argv[2]);
	maxfd= sockfd;
	logInfo("Setup Listening Socket");

	for (i= 3; i < argc; i+= 2) {
		char *inetRes= (char*) malloc(sizeof(char) * BUFSIZE);
		std::string ip= setupConnection(argv[i], argv[i+1]);
		vec_serverAddrs.push_back(ip);
	}

	logInfo("Waiting for requests");	

	logInfo(format);
	free(format);

	int numbytes= 0;
	while (true) {
		// Prepare to select`
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
	char *addrCstr= (char*) malloc(sizeof(char)*BUFSIZE);
	inet_ntop(AF_INET, &(addr->sin_addr), addrCstr, BUFSIZE);
	std::string ipStr= addrCstr;
	free(addrCstr);
	
	char port[6];
	snprintf(port, 6, "%hu", ntohs(addr->sin_port));
	ipStr= (ipStr + ":" + port);
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
	std::string userStr= username;
	
	std::string lastAddr=addrToString((struct sockaddr_in*)&lastUser);
	
	std::string storedAddr= map_userToAddr[userStr];
	
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
	map_userToAddr[userStr]= lastAddr;
	
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

	char *format= (char*) malloc(sizeof(char) * BUFSIZE);

	std::string userStr= username;
	std::string chanStr= channel;
	std::pair<std::multimap<std::string,std::string>::iterator,std::multimap<std::string,std::string>::iterator> ii;
	std::multimap<std::string,std::string>::iterator it;
	bool seen= false;
	
	// Check for channel dups in userToChan; add if none
	ii= map_userToChan.equal_range(userStr);
	for (it= ii.first; it != ii.second; it++) {
		if (it->second == chanStr) { 
			seen= true;
			snprintf(format, BUFSIZE, "User %s already belongs to channel %s; ignoring join request", username, channel);
			logWarning(format);
			return false; 
		}
	}
	if (!seen) { 
		map_userToChan.insert( std::pair<std::string,std::string>(userStr, chanStr) ); 
		snprintf(format, BUFSIZE, "Adding user %s to channel %s", username, channel);
		logInfo(format);
		msg_s2s_join(channel);
	}

	// Check for username dups in chanToUser; add if none
	seen= false;
	ii= map_chanToUser.equal_range(chanStr);
	for (it= ii.first; it != ii.second; it++) {
		if (it->second == userStr) {
			seen= true;
			break;
		}
	}
	if (!seen) { 
		map_chanToUser.insert( std::pair<std::string,std::string>(chanStr, userStr) ); 
	}

	free(format);
	return true;
}

long long genuid() {
	long long uid= 0LL;
	int fd= open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		return 0;
	}
	
	int numbytes= read(fd, &uid, 8);
	if (numbytes == 0) {
		return 0;
	}
	return uid;
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

void logReceivedS2S(int type, const char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", REQ_STR[type], msg);
	log(stdout, "[RECV] ::", format, ANSI_COLOR_BLUE);
	free(format);
}

void logSent(int type, const char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", TXT_STR[type], msg);
	log(stdout, "[SEND] ::", format, ANSI_COLOR_MAGENTA);
	free(format);	
}

void logSentS2S(int type, const char *msg) {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "[%s] \"%s\"", REQ_STR[type], msg);
	log(stdout, "[SS2S] ::", format, ANSI_COLOR_BLUE);
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
	txt->txt_type= htonl(TXT_ERROR);
	strncpy(txt->txt_error, msg, SAY_MAX);

	int result= sendToLast((struct text*) txt, sizeof(struct text_error));
	free(txt);
	logSent(TXT_ERROR, msg);
	return result;
}

int msg_list() {
	char *format= (char*) malloc(BUFSIZE * sizeof(char));

	std::set<std::string> channels;
	std::multimap<std::string,std::string>::iterator it;
	std::set<std::string,std::string>::iterator sit;
	for (it= map_chanToUser.begin(); it != map_chanToUser.end(); it++) {
		channels.insert(it->first);
	}

	if (channels.size() == 0) {	// Should be impossible to reach, really.
		msg_error("No channels exist on this server");
		return false;
	}

	struct text_list *txt= (struct text_list*) malloc(sizeof(struct text_list) + channels.size()*sizeof(struct channel_info));
	txt->txt_type= htonl(TXT_LIST);
	txt->txt_nchannels= htonl(channels.size());

	int i= 0;
	for (sit= channels.begin(); sit != channels.end(); sit++) {
		strncpy(txt->txt_channels[i++].ch_channel, sit->c_str(), CHANNEL_MAX);
	}

	int result= sendToLast((struct text*)txt, sizeof(struct text_list) + channels.size()*sizeof(struct channel_info));
	if (result == true) {
		snprintf(format, BUFSIZE, "%d channels sent", channels.size());
		logSent(TXT_LIST, format);
	}

	free(format);
	free(txt);
	return true;
}

int msg_s2s_join(const char *channel) {
	struct request_s2s_join *req= (struct request_s2s_join*) malloc(sizeof(struct request_s2s_join));
	req->req_type= htonl(REQ_S2S_JOIN);
	strncpy(req->req_channel, channel, CHANNEL_MAX);

	int result= s2s_forward((struct request*) req, sizeof(struct request_s2s_join));
	if (result == true) {
	logSentS2S(REQ_S2S_JOIN, channel);
	}
	free(req);
	return result;
}

int msg_s2s_leave(char *channel) {
	if (channel == NULL) {return false;}
	
	struct request_s2s_leave *req= (struct request_s2s_leave*) malloc(sizeof(struct request_s2s_leave));
	req->req_type= htonl(REQ_S2S_LEAVE);
	strncpy(req->req_channel, channel, CHANNEL_MAX);
	
	int result= s2s_sendToLast((struct request*) req, sizeof(struct request_s2s_join));
	free(req);
	return result;
}

int msg_s2s_say(struct request_s2s_say* req) {
	if (req == NULL) {
		return false;
	}
	// Register the uid before broadcasting
	set_sayList.insert(req->uid);

	int result= s2s_forward((struct request*) req, sizeof(struct request_s2s_say));
	if (result == true) {
		logSentS2S(REQ_S2S_SAY, req->req_text);
	}
	return result;
}

int msg_s2s_say(char *username, char *channel, char *msg) {
	if (username == NULL || channel == NULL || msg == NULL) {
		return false;
	}

	struct request_s2s_say *req= (struct request_s2s_say*) malloc(sizeof(struct request_s2s_say));
	req->req_type= htonl(REQ_S2S_SAY);
	strncpy(req->req_channel, channel, CHANNEL_MAX);
	strncpy(req->req_username, username, USERNAME_MAX);
	strncpy(req->req_text, msg, SAY_MAX);
	req->uid= genuid();

	int result= msg_s2s_say(req);
	free(req);
	return result;
}

int msg_say(const char *channel, const char *fromUser, const char *msg, const struct sockaddr *addr) {
	struct text_say *txt= (struct text_say*) malloc(sizeof(struct text_say));
	char *format= (char*) malloc(sizeof(char)*BUFSIZE);

	txt->txt_type= htonl(TXT_SAY);
	strncpy(txt->txt_channel, channel, CHANNEL_MAX);
	strncpy(txt->txt_username, fromUser, USERNAME_MAX);
	strncpy(txt->txt_text, msg, SAY_MAX);

	int result= sendMessage(addr, sizeof(struct sockaddr_in), (struct text*)txt, sizeof(struct text_say));
	if (result == true) {
		snprintf(format, BUFSIZE, "[%s][%s] %s", channel, fromUser, msg);
		logSent(TXT_SAY, format);
	}

	free(txt);
	free(format);
	return result;
}

int msg_who(const char *channel) {
	if (channel == NULL) {
		logError("msg_who: channel was NULL");
		return false;
	}

	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	std::pair<std::multimap<std::string,std::string>::iterator,std::multimap<std::string,std::string>::iterator> ii;
	std::multimap<std::string,std::string>::iterator it;
	std::string chanStr= channel;

	int numUsers= 0;
	ii= map_chanToUser.equal_range(chanStr);
	for (it= ii.first; it != ii.second; it++) {
		++numUsers;
	}

	if (numUsers == 0) {
		snprintf(format, BUFSIZE, "Channel %s does not exist", channel);
		msg_error(format);
		return false;
	}

	struct text_who *txt= (struct text_who*) malloc(sizeof(struct text_who) + numUsers*sizeof(struct user_info));
	txt->txt_type= htonl(TXT_WHO);
	txt->txt_nusernames= htonl(numUsers);
	strncpy(txt->txt_channel, channel, CHANNEL_MAX);
	int i= 0;
	ii= map_chanToUser.equal_range(chanStr);
	
	for (it=ii.first; it != ii.second; it++) {
		strncpy(txt->txt_users[i++].us_username, it->second.c_str(), USERNAME_MAX);
	}

	int result= sendToLast((struct text*)txt, sizeof(struct text_who) + numUsers*sizeof(struct user_info));
	if (result == true) {
		snprintf(format, BUFSIZE, "<%s> %d users sent", channel, numUsers);
		logSent(TXT_WHO, format);
	}

	free(txt);
	free(format);
	return result;
}


struct sockaddr *new_stringToAddr(std::string straddr) {
	struct sockaddr_in *sa= (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
	char *tok= (char*) malloc(sizeof(char)*BUFSIZE);
	strcpy(tok, straddr.c_str());
		
	char *ip= strtok(tok, ":");
	char *port= strtok(NULL, ":");
	/*u_short p= (u_short) atoi(port);

	inet_pton(AF_INET, ip, &(sa->sin_addr));
	sa->sin_port= p;
	sa->sin_family= AF_INET;

	return sa;*/

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_DGRAM;
	int result= getaddrinfo(ip, port, &hints, &res);

	return res->ai_addr;
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
	std::string userStr= addrToUser((struct sockaddr_in*)&lastUser);
	std::string chan= req->req_channel;
	set_chanList.insert(chan);
	return addUserToChannel(userStr.c_str(), req->req_channel);
}

int recv_keepAlive(struct request_keep_alive *req) {
	if (req == NULL) { return false; }
	logReceived(REQ_KEEP_ALIVE, "");
	return true;
}

int recv_leave(struct request_leave *req) {
	logReceived(REQ_LEAVE, req->req_channel);
	std::string userStr= addrToUser((struct sockaddr_in*)&lastUser);
	char *userCstr= (char*) malloc(sizeof(char) * BUFSIZE);
	strcpy(userCstr, userStr.c_str());

	int result= removeUserFromChannel(userCstr, req->req_channel);
	free(userCstr);
	return result;
}

int recv_list(struct request_list *req) {
	if (req == NULL) { return false; }
	logReceived(REQ_LIST, "");
	return msg_list();
}

int recv_login(struct request_login *req) {
	logReceived(REQ_LOGIN, req->req_username);
	addUser(req->req_username);
	return true;
}

int recv_logout(struct request_logout *req) {
	if (req == NULL) {return false;}
	logReceived(REQ_LOGOUT, "");

	int result= false;
	std::string userStr= addrToUser((struct sockaddr_in*)&lastUser);
	std::pair<std::multimap<std::string,std::string>::iterator,std::multimap<std::string,std::string>::iterator> ii;
	std::multimap<std::string,std::string>::iterator it;
	ii= map_userToChan.equal_range(userStr);

	char *userCstr= (char*) malloc(sizeof(char) * BUFSIZE);
	char *channel=  (char*) malloc(sizeof(char) * BUFSIZE);
	strcpy(userCstr, userStr.c_str());


	for (it= ii.first; it != ii.second; ++it) {
		strcpy(channel, it->second.c_str());
		if (channel == NULL) return false;
		result= removeUserFromChannel(userCstr, channel) 
				&& result;
	}

	result= removeLastUser() && result;
	free(userCstr);
	free(channel);
	return result;
}

int recv_s2s_join(struct request_s2s_join *req) {
	logReceived(REQ_S2S_JOIN, req->req_channel);
	std::string chan= req->req_channel;
	if (set_chanList.find(chan) == set_chanList.end()) {
		set_chanList.insert(chan);
		return msg_s2s_join(req->req_channel);
	}
	return false;
}

int recv_s2s_say(struct request_s2s_say *req) {
	struct request_say* sreq= (struct request_say*) malloc(sizeof(struct request_say));
	strncpy(sreq->req_channel, req->req_channel, CHANNEL_MAX);
	strncpy(sreq->req_text, req->req_text, SAY_MAX);

	if (set_sayList.find(req->uid) == set_sayList.end()) {	
		logReceived(REQ_S2S_SAY, req->req_text);
		int result= recv_say(sreq, req->uid, req->req_username);
		free(sreq);
		return result;
	}
	msg_s2s_leave(req->req_channel);
	return false;
}

int recv_say(struct request_say *req, long long uid, char *uname) {
	char *msg= (char*) malloc(BUFSIZE * sizeof(char));
	if (uname == NULL) {
		std::string fromUser= addrToUser((struct sockaddr_in*)&lastUser);
		uname= (char*) malloc(sizeof(char) * BUFSIZE);
		strncpy(uname, fromUser.c_str(), USERNAME_MAX);

		sprintf(msg, "<%s> %s", req->req_channel, req->req_text);
		logReceived(REQ_SAY, msg);
		free(msg);
	}
	
	// for every channel the user is a member of...
	std::pair<std::multimap<std::string,std::string>::iterator,std::multimap<std::string,std::string>::iterator> cii;
	std::multimap<std::string,std::string>::iterator cit;

	std::string req_channel= req->req_channel;
	cii= map_chanToUser.equal_range(req_channel);
	
	for (cit= cii.first; cit != cii.second; cit++) {
		std::string userStr= cit->second;

		std::string userAddr= map_userToAddr[userStr];
		char *tok= (char*) malloc(sizeof(char)*BUFSIZE);
		strcpy(tok, userAddr.c_str());
		
		char *ip= strtok(tok, ":");
		char *port= strtok(NULL, ":");
		u_short p= (u_short) atoi(port);

		struct sockaddr_in sa;
		inet_pton(AF_INET, ip, &(sa.sin_addr));
		sa.sin_port= p;
		sa.sin_family= AF_INET;

		msg_say(req->req_channel, uname, req->req_text, (struct sockaddr*)&sa);
		free(tok);
	}

	if (uid == 0) {
		msg_s2s_say(uname, req->req_channel, req->req_text);
	} else {
		struct request_s2s_say* sreq= (struct request_s2s_say*) malloc(sizeof(struct request_s2s_say));
		strncpy(sreq->req_username, uname, USERNAME_MAX);
		strncpy(sreq->req_channel, req->req_channel, CHANNEL_MAX);
		strncpy(sreq->req_text, req->req_text, SAY_MAX);
		sreq->uid= uid;
		sreq->req_type= htonl(REQ_S2S_SAY);
		msg_s2s_say(sreq);
		free(sreq);
	}

	
	
		// for every user in that channel...
	
			// sendto their stored address (hopefully)...
	return false; //msg_say(req->;
}

int recv_who(struct request_who *req) {
	logReceived(REQ_WHO, req->req_channel);
	return msg_who(req->req_channel);
}
int removeLastUser() {
	std::string addrStr= addrToString((struct sockaddr_in*)&lastUser);
	std::string userStr= map_addrToUser[addrStr];
	if (userStr == "") { // Unusual state; no user to remove
		logError("removeLastUser: Unknown source record to remove");
		return false;
	}

	map_addrToUser.erase(addrStr);
	map_userToAddr.erase(userStr);
	char *format= (char*) malloc(BUFSIZE * sizeof(char));
	snprintf(format, BUFSIZE, "Logging out user %s", userStr.c_str());
	logInfo(format);
	free(format);

	return true;	
}

int removeUserFromChannel(const char *username, const char *channel) {
	if (username == NULL) {
		logError("removeUserFromChannel: username was NULL");
		return false;
	} else if (channel == NULL || strlen(channel) > CHANNEL_MAX) {
		logError("removeUserFromChannel: channel was NULL");
		return false;
	}

	std::string userStr= username;
	std::string chanStr= channel;
	char *format= (char*) malloc(sizeof(char) * BUFSIZE);

	std::pair<std::multimap<std::string,std::string>::iterator,std::multimap<std::string,std::string>::iterator> ii;
	std::multimap<std::string,std::string>::iterator it;
	bool seen= false;	

	// Remove first matching chanel from userToChan
	ii= map_userToChan.equal_range(userStr);
	for (it= ii.first; it != ii.second; it++) {
		if (it->second == chanStr) {
			seen= true;
			snprintf(format, BUFSIZE, "Removed channel %s from user %s", it->second.c_str(), it->first.c_str());
			map_userToChan.erase(it);
			logInfo(format);
			break;
		}
	}
	if (!seen) {
		snprintf(format, BUFSIZE, "While removing, did not find channel %s recorded for user %s", channel, username);
		logWarning(format);
	}

	// Remove first matching user from chanToUser
	ii= map_chanToUser.equal_range(chanStr);
	seen= false;
	for (it= ii.first; it != ii.second; ++it) {
		if (it->second == userStr) {
			seen= true;
			snprintf(format, BUFSIZE, "Removed user %s from channel %s", it->second.c_str(), it->first.c_str());
			map_chanToUser.erase(it);
			logInfo(format);
			break;
		}
	}
	if (!seen) {
		snprintf(format, BUFSIZE, "While removing, did not find user %s recorded for channel %s", username, channel);
		logWarning(format);
	}

	free(format);
	return true;
}

int s2s_broadcast(struct request *msg, int msglen) {
	std::string lastAddr= addrToString((sockaddr_in*)&lastUser);
	unsigned int i, result= true;
	if (vec_serverAddrs.size() == 0) { return false; }
	for (i= 0; i < vec_serverAddrs.size(); i++) {
		const struct sockaddr *sa= new_stringToAddr(vec_serverAddrs[i]);
		result= s2s_send(sa, sizeof(struct sockaddr_in), msg, msglen) && result;
	}
	return result;
}

int s2s_forward(struct request *msg, int msglen) {
	std::string lastAddr= addrToString((struct sockaddr_in*) &lastUser);
	unsigned int i, result= true;
	if (vec_serverAddrs.size() == 0) {return false;}
	for (i= 0; i < vec_serverAddrs.size(); i++) {
		if (lastAddr != vec_serverAddrs[i]) {
			const struct sockaddr *sa= new_stringToAddr(vec_serverAddrs[i]);
			result= s2s_send(sa, sizeof(struct sockaddr_in), msg, msglen) && result;
		}
	}
	return result;
}

int s2s_send(const struct sockaddr *addr, size_t addrlen, struct request *msg, int msglen) {
	int result= sendto(sockfd, msg, msglen, 0, addr, addrlen);
	if (result == -1) {
		perror("send");
		return false;
	}
	return true;
}

int s2s_sendToLast(struct request *msg, int msglen) {
	return s2s_send((struct sockaddr*)&lastUser, lastSize, msg, msglen);
}

int sendMessage(const struct sockaddr *addr, size_t addrlen, struct text *msg, int msglen) {
	int result= sendto(sockfd, msg, msglen, 0, addr, addrlen);
	if (result == -1) {
		perror("sendto: ");
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

std::string setupConnection(char *addr, char *port) {
	int result= 0;
	
	// Setup hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_DGRAM;

	struct addrinfo *sInfo;

	// Fetch address info struct
	if ((result= getaddrinfo(addr, port, &hints, &sInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return "";
	}

	return addrToString((struct sockaddr_in*)&sInfo->ai_addr);
}

int switchRequest(struct request* req, int len) {
	char *warning= (char*) malloc(sizeof(char)*BUFSIZE);
	int result= false;
	req->req_type= ntohl(req->req_type);

	if (req->req_type != REQ_S2S_JOIN && req->req_type != REQ_S2S_LEAVE && req->req_type != REQ_S2S_SAY) {
		if (addrToUser((struct sockaddr_in*)&lastUser) == "" && req->req_type != REQ_LOGIN) {
			snprintf(warning, BUFSIZE, "Received request type %d from unknown user; ignoring", req->req_type);
			logWarning(warning);
			free(warning);
			return result;
		}
	}

	switch( req->req_type ) {
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
		result= recv_say( (struct request_say*) req, 0, NULL);
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

	case REQ_S2S_JOIN:
		if (sizeof(struct request_s2s_join) != len) {
			snprintf(warning, BUFSIZE, "Received keepalive request; expected %d bytes but was %d bytes", sizeof(struct request_keep_alive), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_s2s_join( (struct request_s2s_join*) req);
		break;

	case REQ_S2S_LEAVE:
		if (sizeof(struct request_s2s_leave) != len) {
			snprintf(warning, BUFSIZE, "Received s2s_leave request; expected %d bytes byt was %d bytes", sizeof(struct request_s2s_say), len);
			logWarning(warning);
			result= false;
			break;
		}
		logReceived(REQ_S2S_LEAVE, "");
		break;

	case REQ_S2S_SAY:
		if (sizeof(struct request_s2s_say) != len) {
			snprintf(warning, BUFSIZE, "Received s2s_say request; expecrted %d bytes but was %d bytes", sizeof(struct request_s2s_say), len);
			logWarning(warning);
			result= false;
			break;
		}
		result= recv_s2s_say( (struct request_s2s_say*) req);
		break;

	default:
		snprintf(warning, BUFSIZE, "Received unknown request type %d of size %d bytes", req->req_type, len);
		logWarning(warning);
		break;
	}
	free(warning);
	return result;
}
