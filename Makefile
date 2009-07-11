CC = gcc
CFLAGS = -I inc $(shell pkg-config --cflags glib-2.0)
LIBS=$(shell pkg-config --libs glib-2.0)

sasl:
	$(CC) -c src/sasl_auth.c $(CFLAGS)

net:
	$(CC) -c src/net.c $(CFLAGS)

utils:
	$(CC) -c src/utils.c $(CFLAGS)

delivery:
	$(CC) -c src/delivery.c $(CFLAGS)

protocolhandler:
	$(CC) -c src/protocol_handler.c $(CFLAGS)

cherami:
	$(CC) -c src/cherami.c $(CFLAGS)

all: net utils cherami sasl delivery protocolhandler
	$(CC) -o cherami *.o $(LIBS)
	
clean:
	@rm -f *.o
	@rm cherami
