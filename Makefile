#
# MyLocalHost makefile
# gygax@practicomp.ch
#
# TODO: find a way to update third-party libraries
#

CFLAGS += -I third-party/websockify
# LDFLAGS += -shared

SRC = mylocalhost.c

OBJ = $(SRC:.c=.o)

all: mylocalhost

mylocalhost: $(OBJ)
	$(CC) $(LDFLAGS) $^ -lwebserver -lwebsocket -lwsproxy -o $@
	
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
    
clean:
	rm -f *.o mylocalhost
