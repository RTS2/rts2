# $Id$

noinst_LIBRARIES = liburvc.a
noinst_HEADERS = mardrv.h ccd_macros.h

liburvc_a_SOURCES = camera.c mario.c marecp.c marmicro.c ccd_imaging.c ccd_tracking.c
liburvc_a_CFLAGS = -O3 -I$(CFITSIO) -I$(READLINE) -Wall -Wshadow -g -march=i586 -I../../../include #-DDEBUG
#-march=i586 -ggdb 
#--pedantic
