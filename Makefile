CC = gcc
CFLAGS = -I inc 
LIBS= -lm 

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
