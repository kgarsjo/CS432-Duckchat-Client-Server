CC=g++

CFLAGS=-Wall -W -g

LINUXLIBS=-lnsl
SOLARISLIBS=-lsocket $(LINUXLIBS)

all: client server test

client: client.c raw.c
	$(CC) client.c raw.c $(SOLARISLIBS) $(CFLAGS) -c
	$(CC) client.o raw.o $(SOLARISLIBS) $(CFLAGS) -o client

server: server.c 
	$(CC) server.c $(SOLARISLIBS) $(CFLAGS) -o server

test: test.c
	$(CC) test.c $(SOLARISLIBS) $(CFLAGS) -o test

linux:
	$(CC) client.c raw.c $(LINUXLIBS) $(CFLAGS) -o client
	$(CC) server.c $(LINUXLIBS) $(CFLAGS) -o server

clean:
	rm -f client server *.o
	rm -f test

