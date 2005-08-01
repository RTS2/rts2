SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy

noinst_HEADERS = focuser.h

rts2_focusd_optec_SOURCES = focusd.cpp optec.cpp
rts2_focusd_optec_LDFLAGS = ../utils/librts2block.a

rts2_focusd_robofocus_SOURCES = focusd.cpp robofocus.cpp
rts2_focusd_robofocus_LDFLAGS = ../utils/librts2block.a

rts2_focusd_dummy_SOURCES = focusd.cpp dummy.cpp
rts2_focusd_dummy_LDFLAGS = ../utils/librts2block.a
