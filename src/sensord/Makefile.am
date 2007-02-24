bin_PROGRAMS = rts2-sensor-dummy

noinst_HEADERS = sensord.h

LDADD = ../utils/librts2block.a

rts2_sensor_dummy_SOURCES = sensord.cpp dummy.cpp

if GPIBLIB

bin_PROGRAMS += rts2-sensor-cryocon rts2-sensor-keithley-6485

rts2_sensor_cryocon_SOURCES = sensord.cpp cryocon.cpp
rts2_sensor_cryocon_LDFLAGS = @LIB_GPIB@

rts2_sensor_keithley_6485_SOURCES = sensord.cpp keithley-6485.cpp
rts2_sensor_keithley_6485_LDFLAGS = @LIB_GPIB@

endif
