CC=gcc

CFLAGS = -O3 -I$(CFITSIO) -I$(READLINE) -Wall -Wshadow -g -march=i586 -I../../../include #-DDEBUG
#-march=i586 -ggdb 
#--pedantic

TARGET= ./
OBJS= parccd.o pardrv.o pardrvclient.o sbig.o
#OBJS= pardrvclient.o mario.o sbig.o marmicro.o
#SOLIBS= -lsbig
TO_CLEAN= *.o core *.log *.out *~
#marmicro.o 

all: marmicro.o mario.o marecp.o ccd_imaging.o ccd_tracking.o camera.o

mario.o: mario.c mardrv.h
marecp.o: marecp.c mardrv.h
marmicro.o: marmicro.c mardrv.h
ccd_imaging.o: ccd_imaging.c mardrv.h ccd_macros.h
ccd_tracking.o: ccd_tracking.c mardrv.h ccd_macros.h

clean: 
	rm -f $(TO_CLEAN)

indent:
	indent -pmt *.[ch]

mrproper:
	rm -f $(TO_CLEAN) urvc *~
