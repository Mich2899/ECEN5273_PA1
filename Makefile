CC=gcc
CFLAGS = -Wall -Werror
VPATH=./server:./client

all:	udp_server udp_client

udp_server:	udp_server.c
	$(CC) $(CFLAGS) -o server/udp_server server/udp_server.c

udp_client:	udp_client.c
	$(CC) $(CFLAGS) -o client/udp_client client/udp_client.c

clean:
	rm -rf server/server client/client