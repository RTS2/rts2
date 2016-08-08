lib_LTLIBRARIES = librts2vermes.la

AM_CFLAGS=@NOVA_CFLAGS@  -I../../../include

librts2vermes_la_SOURCES= move_door.c \
	move_to_target_az.c barcodereader.c ssd650v_comm.c \
	bisync.c util.c serial.c oak_comm.c
librts2vermes_la_LIBADD = @LIB_NOVA@ @LIB_PTHREAD@
