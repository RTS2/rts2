lib_LTLIBRARIES = librts2pier-collision.la librts2tellx200.la

librts2tellx200_la_SOURCES = tellx200.cpp tellx200gps.cpp hms.c
librts2tellx200_la_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
librts2tellx200_la_CFLAGS = @NOVA_CFLAGS@ -I../../include
librts2tellx200_la_LIBADD = ../rts2tel/librts2tel.la

librts2pier_collision_la_SOURCES = pier-collision.cpp
librts2pier_collision_la_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
librts2pier_collision_la_LIBADD = @LIB_NOVA@
