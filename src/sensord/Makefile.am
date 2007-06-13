bin_PROGRAMS = rts2-sensor-dummy rts2-sensor-ms257

noinst_HEADERS = sensord.h

LDADD = ../utils/librts2block.a

rts2_sensor_dummy_SOURCES = sensord.cpp dummy.cpp

rts2_sensor_ms257_SOURCES = sensord.cpp ms257.cpp

if GPIBLIB

bin_PROGRAMS += rts2-sensor-cryocon rts2-sensor-keithley-6485

rts2_sensor_cryocon_SOURCES = sensord.cpp cryocon.cpp
rts2_sensor_cryocon_LDFLAGS = @LIB_GPIB@

rts2_sensor_keithley_6485_SOURCES = sensord.cpp keithley-6485.cpp
rts2_sensor_keithley_6485_LDFLAGS = @LIB_GPIB@

endif

if A3200

bin_PROGRAMS += rts2-sensor-a3200

rts2_sensor_a3200_SOURCES = sensord.cpp a3200.cpp
rts2_sensor_a3200_CXXFLAGS = @INC_A3200@
rts2_sensor_a3200_LDADD = @LIB_A3200@ ../utils/librts2block.a

endif
