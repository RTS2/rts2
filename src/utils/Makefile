CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm

all: test devdem.o

test: hms.o test.o

clean: 
	rm -rf *.o test *~

indent:
	indent *.c *.h

