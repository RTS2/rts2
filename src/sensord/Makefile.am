bin_PROGRAMS = rts2-sensor-dummy rts2-sensor-ms257 rts2-sensor-newport-lamp rts2-sensor-phytron \
	rts2-sensor-ds21 rts2-sensor-mrakomer rts2-sensor-pixy rts2-sensor-davis \
	rts2-sensor-fram-weather rts2-sensor-bart-rain \
	rts2-sensor-osn rts2-sensor-system rts2-sensor-apcups rts2-sensor-nut \
	rts2-sensor-cloud4 rts2-sensor-brooks-356

noinst_HEADERS = sensord.h sensorgpib.h conngpib.h davisudp.h fram-weather.h framudp.h davis.h

EXTRA_DIST = 

noinst_LIBRARIES = librts2sensord.a

if GPIBLIB

librts2sensord_a_SOURCES = sensord.cpp sensorgpib.cpp conngpib.cpp

else

librts2sensord_a_SOURCES = sensord.cpp

EXTRA_DIST += sensorgpib.cpp conngpib.cpp

endif


LDADD = librts2sensord.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

rts2_sensor_dummy_SOURCES = dummy.cpp

rts2_sensor_ms257_SOURCES = ms257.cpp

rts2_sensor_newport_lamp_SOURCES = newport-lamp.cpp

rts2_sensor_phytron_SOURCES = phytron.cpp

rts2_sensor_ds21_SOURCES = ds21.cpp

rts2_sensor_mrakomer_SOURCES = mrakomer.cpp

rts2_sensor_cloud4_SOURCES = cloud4.cpp

rts2_sensor_pixy_SOURCES = pixy.cpp

rts2_sensor_davis_SOURCES = davis.cpp davisudp.cpp

rts2_sensor_fram_weather_SOURCES = fram-weather.cpp framudp.cpp

rts2_sensor_bart_rain_SOURCES = bart-rain.cpp

rts2_sensor_mrakomer_move_SOURCES = mrakomer-move.cpp

rts2_sensor_osn_SOURCES = osn.cpp

rts2_sensor_system_SOURCES = system.cpp

rts2_sensor_apcups_SOURCES = apcups.cpp

rts2_sensor_nut_SOURCES = nut.cpp

rts2_sensor_brooks_356_SOURCES = brooks-356.cpp

if COMEDI
bin_PROGRAMS += rts2-sensor-bootes2

rts2_sensor_bootes2_SOURCES = bootes2.cpp
rts2_sensor_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_sensor_bootes2_CPPFLAGS = @INC_COMEDI@

else
EXTRA_DIST += bootes2.cpp
endif

if GPIBLIB

LDADD += @LIB_GPIB@

bin_PROGRAMS += rts2-sensor-cryocon rts2-sensor-keithley-6485 \
	rts2-sensor-keithley-487

rts2_sensor_cryocon_SOURCES = cryocon.cpp

rts2_sensor_keithley_6485_SOURCES = keithley-6485.cpp

rts2_sensor_keithley_487_SOURCES = keithley-487.cpp

else

EXTRA_DIST += cryocon.cpp keithley-6485.cpp keithley-487.cpp

endif

if A3200

bin_PROGRAMS += rts2-sensor-a3200

rts2_sensor_a3200_SOURCES = a3200.cpp
rts2_sensor_a3200_CXXFLAGS = @INC_A3200@
rts2_sensor_a3200_LDFLAGS = @LIB_A3200@ ../utils/librts2block.a

else

EXTRA_DIST += a3200.cpp

endif
