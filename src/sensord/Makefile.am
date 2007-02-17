bin_PROGRAMS = rts2-sensor-dummy

noinst_HEADERS = sensord.h

LDADD = ../utils/librts2block.a

rts2_sensor_dummy_SOURCES = sensord.cpp dummy.cpp

if GPIBLIB

bin_PROGRAMS += rts2-sensor-cryocon

rts2_sensor_cryocon_SOURCES = sensord.cpp cryocon.cpp
rts2_sensor_cryocon_LDFLAGS = @LIB_GPIB@

endif
