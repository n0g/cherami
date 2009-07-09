CC = gcc
CFLAGS = -I inc
VERSION = "\"cherami 0.1\""

sasl:
	$(CC) -c src/sasl_auth.c

net:
	$(CC) -c src/net.c $(CFLAGS) -DVERSION=$(VERSION)

all: net sasl
	$(CC) -o cherami *.o
	
clean:
	@rm -f *.o
