CFLAGS=-Wall -O3
LDFLAGS=-lm


all: para.a

para.a: libmks3.o te.o
	$(AR) rcs $@ $^

libmks3.so: libmks3.c libmks3.h
	cc $(CFLAGS) -shared -o libmks3.so libmks3.c

libmks3.o: libmks3.c 
	cc $(CFLAGS) -c -o libmks3.o libmks3.c

te.o: te.c
	cc $(CFLAGS) -I../../../include -c -o te.o te.c

clean: 
	$(RM) -f *~ *.so *.o te


