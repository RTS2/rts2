LIBS = /usr/local/lib/libwcs.a -lm
CFLAGS = -Wall -Wshadow -g

all: wcs2db

wcs2db: wcs2db.c 
	$(CC) $(CFLAGS) -o wcs2db wcs2db.c $(LIBS)

clean:
	$(RM) *.o wcs2db
