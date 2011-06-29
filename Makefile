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
    
# webserver.pseudo: $(WSFSUBDIR)/webserver/libwebserver.a
# 	pushd $(WSFSUBDIR)/webserver
# 	make
# 	sudo make install
# 	popd
# 	echo touch $@
# 	touch $@
# 
# websocket.pseudo: $(WSFSUBDIR)/websocket/libwebsocket.a
# 	pushd $(WSFSUBDIR)/websocket
# 	make
# 	sudo make install
# 	popd
# 	touch $@
# 
# wsproxy.pseudo: $(WSFSUBDIR)/wsproxy/libwsproxy.a
# 	pushd $(WSFSUBDIR)/wsproxy
# 	make
# 	sudo make install
# 	popd	
# 	touch $@

clean:
	rm -f *.o mylocalhost
