bin_PROGRAMS = rts2-sensor-dummy rts2-sensor-ms257 rts2-sensor-newport-lamp rts2-sensor-phytron \
	rts2-sensor-ds21 rts2-sensor-mrakomer rts2-sensor-pixy rts2-sensor-davis \
	rts2-sensor-fram-weather rts2-sensor-bart-rain \
	rts2-sensor-osn rts2-sensor-system rts2-sensor-apcups rts2-sensor-nut \
	rts2-sensor-cloud4 rts2-sensor-brooks-356 rts2-sensor-bootes1 rts2-sensor-aag rts2-sensor-pwc \
	rts2-sensor-hygrowin rts2-sensor-fram rts2-sensor-flwo-weather rts2-sensor-thorlaser \
	rts2-sensor-arduino

noinst_HEADERS = sensord.h sensorgpib.h conngpib.h conngpiblinux.h conngpibenet.h davisudp.h fram-weather.h framudp.h davis.h aag.h pwc-ioctl.h tps534-oak.h

noinst_LIBRARIES = librts2sensord.a

EXTRA_DIST = 

if GPIBLIB

librts2sensord_a_SOURCES = sensord.cpp sensorgpib.cpp conngpib.cpp conngpiblinux.cpp conngpibenet.cpp

else

librts2sensord_a_SOURCES = sensord.cpp

EXTRA_DIST += sensorgpib.cpp conngpib.cpp conngpiblinux.cpp conngpibenet.cpp

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

rts2_sensor_bootes1_SOURCES = bootes1.cpp

rts2_sensor_aag_SOURCES = aag.cpp

rts2_sensor_pwc_SOURCES = pwc.cpp

rts2_sensor_hygrowin_SOURCES = hygrowin.cpp

rts2_sensor_fram_SOURCES = fram.cpp

if SUNOS
EXTRA_DIST += tps534.cpp tps534-oak.c
else
bin_PROGRAMS += rts2-sensor-tps534

rts2_sensor_tps534_SOURCES = tps534.cpp tps534-oak.c 
rts2_sensor_tps534_LDADD =  @LIB_PTHREAD@ ${LDADD} ../dome/librts2oak.a
endif

rts2_sensor_flwo_weather_SOURCES = flwo-weather.cpp

rts2_sensor_thorlaser_SOURCES = thorlaser.cpp

rts2_sensor_arduino_SOURCES = arduino.cpp

if COMEDI
bin_PROGRAMS += rts2-sensor-bootes2 rts2-sensor-lpnhe

rts2_sensor_bootes2_SOURCES = bootes2.cpp
rts2_sensor_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_sensor_bootes2_CPPFLAGS = @INC_COMEDI@

rts2_sensor_lpnhe_SOURCES = lpnhe.cpp
rts2_sensor_lpnhe_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_sensor_lpnhe_CPPFLAGS = @INC_COMEDI@

else
EXTRA_DIST += bootes2.cpp
endif

if GPIBLIB

LDADD += @LIB_GPIB@

bin_PROGRAMS += rts2-sensor-cryocon rts2-sensor-keithley-scpi \
	rts2-sensor-keithley-487 rts2-sensor-lakeshore-325 rts2-sensor-triax \
	rts2-sensor-mm2500 rts2-sensor-lakeshore-330

rts2_sensor_cryocon_SOURCES = cryocon.cpp

rts2_sensor_keithley_scpi_SOURCES = keithley-scpi.cpp

rts2_sensor_keithley_487_SOURCES = keithley-487.cpp

rts2_sensor_lakeshore_325_SOURCES = lakeshore-325.cpp

rts2_sensor_lakeshore_330_SOURCES = lakeshore-330.cpp

rts2_sensor_triax_SOURCES = triax.cpp

rts2_sensor_mm2500_SOURCES = mm2500.cpp

else

EXTRA_DIST += cryocon.cpp keithley-scpi.cpp keithley-487.cpp lakeshore-325.cpp triax.cpp mm2500.cpp lakeshore-330.cpp

endif

if A3200

bin_PROGRAMS += rts2-sensor-a3200

rts2_sensor_a3200_SOURCES = a3200.cpp
rts2_sensor_a3200_CXXFLAGS = @INC_A3200@
rts2_sensor_a3200_LDADD = @LIB_A3200@ ${LDADD}

else

EXTRA_DIST += a3200.cpp

endif
