#ifndef CLIENT_H_
#define CLIENT_H_

int login(char*);
int logout();
int setupSocket(char*, char*);
int sendMessage(char*, int len);

#endif
