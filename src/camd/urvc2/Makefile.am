#Id#

noinst_LIBRARIES = liburvc2.a
noinst_HEADERS = tcf.h urvc.h 

liburvc2_a_SOURCES = io.c micro.c st237.c tcf.c trk.c ccd.c sexif.c st7.c tools.c
liburvc2_a_CFLAGS = -O3 -I$(CFITSIO) -I$(READLINE) -Wall -Wshadow -g -march=i586 -I../../../include #-DDEBUG
