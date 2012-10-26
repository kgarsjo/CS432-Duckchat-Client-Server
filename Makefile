CC=g++

CFLAGS=-Wall -W -g 

LINUXLIBS=-lnsl
SOLARISLIBS=-lsocket $(LINUXLIBS)

all: client server

client: client.c raw.c
	$(CC) client.c raw.c $(SOLARISLIBS) $(CFLAGS) -o client

server: server.c 
	$(CC) server.c $(SOLARISLIBS) $(CFLAGS) -o server

linux:
	$(CC) client.c raw.c $(LINUXLIBS) $(CFLAGS) -o client
	$(CC) server.c $(LINUXLIBS) $(CFLAGS) -o server

clean:
	rm -f client server *.o
	rm -f tests

