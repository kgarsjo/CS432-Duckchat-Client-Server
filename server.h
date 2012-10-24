#ifndef SERVER_H_
#define SERVER_H_

struct user_profile {
	struct sockaddr src_addr;	// The user's source address
	socklen_t addrlen;		// The length of the user's source address
	char *username;			// The user's username
	struct user_profile *next;	// The next user profile
};

int addUser(struct user_profile user);
int removeUser(struct sockaddr src_addr);
int removeUser(char *username);

#endif
