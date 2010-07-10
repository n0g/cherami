CC = gcc
CFLAGS = -I inc 
LIBS= -lm 

all: net utils cherami sasl delivery protocolhandler
	$(CC) -o cherami *.o $(LIBS)

sasl:
	$(CC) -c src/sasl_auth.c $(CFLAGS)

net:
	$(CC) -c src/net.c $(CFLAGS)

utils:
	$(CC) -c src/utils.c $(CFLAGS)

delivery:
	$(CC) -c src/delivery.c $(CFLAGS)

protocolhandler:
	lex -i -Cr smtp.lex
	yacc -d smtp.yacc
	$(CC) -c lex.yy.c $(CFLAGS)
	$(CC) -c y.tab.c $(CFLAGS)

cherami:
	$(CC) -c src/cherami.c $(CFLAGS)

clean:
	@rm -f *.o
	@rm lex.yy.c
	@rm y.tab.c
	@rm y.tab.h
	@rm cherami
