CC = gcc
CFLAGS = -I inc

sasl:
	$(CC) -c src/sasl_auth.c

delivery:
	$(CC) -c src/delivery.c $(CFLAGS)

protocolhandler:
	$(CC) -c src/protocol_handler.c $(CFLAGS)

cherami:
	$(CC) -c src/cherami.c $(CFLAGS)

all: sasl delivery cherami protocolhandler
	$(CC) -o cherami *.o
	
clean:
	@rm -f *.o
	@rm cherami
