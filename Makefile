CC=g++

CFLAGS=-Wall -W -g -Werror 

LOADLIBES=-lsocket -lnsl
TESTLIBS= -lgtest -lgtest_main -lpthread

all: client server

client: client.c raw.c
	$(CC) client.c raw.c $(LOADLIBES) $(CFLAGS) -o client

server: server.c 
	$(CC) server.c $(LOADLIBES) $(CFLAGS) -o server

tests:	tests.c
	$(CC) tests.c $(TESTLIBS) $(CFLAGS) -o tests

clean:
	rm -f client server tests *.o

