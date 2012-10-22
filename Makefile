CC=g++

CFLAGS=-Wall -W -g 

LINUXLIBS=-lnsl
SOLARISLIBS=-lsocket $(LINUXLIBS)

# Defined for test library; requires google C++ testing libs in /usr/lib
TESTLIBS= -lgtest -lgtest_main -lpthread

all: logging client server

client: client.c raw.c
	$(CC) client.c raw.c logging.c $(SOLARISLIBS) $(CFLAGS) -o client

server: server.c 
	$(CC) server.c logging.c$(SOLARISLIBS) $(CFLAGS) -o server

logging: logging.c
	$(CC) -c logging.c
	ar rcs liblogging.a logging.o

# Made separately from [all]; requires google C++ testing libs
tests:	tests.c
	$(CC) tests.c client.c $(TESTLIBS) $(CFLAGS) -o tests

linux: logging
	$(CC) client.c $(LINUXLIBS) $(CFLAGS) -o client
	$(CC) server.c $(LINUXLIBS) $(CFLAGS) -o server
	

clean:
	rm -f client server *.o
	rm -f tests

