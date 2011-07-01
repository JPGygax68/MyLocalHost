#
# MyLocalHost makefile
# gygax@practicomp.ch
#
# TODO: find a way to update third-party libraries
#

SHELL = /bin/bash

WSFSUBDIR = third-party/websockify

CFLAGS += -I $(WSFSUBDIR)

SRC = mylocalhost.c

OBJ = $(SRC:.c=.o)

all: mylocalhost

mylocalhost: $(OBJ) #webserver.pseudo websocket.pseudo wsproxy.pseudo
	$(CC) $(LDFLAGS) $^ -lwebserver -lwebsocket -lwsproxy -o $@
	
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
    
clean:
	rm -f *.o mylocalhost
