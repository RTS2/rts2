bin_PROGRAMS = rts2-sensor-dummy rts2-sensor-multi rts2-sensor-ms257 rts2-sensor-newport-lamp \
	rts2-sensor-phytron rts2-sensor-ds21 rts2-sensor-mrakomer rts2-sensor-pixy rts2-sensor-davis-udp \
	rts2-sensor-fram-weather rts2-sensor-bart-rain rts2-sensor-ms260 rts2-sensor-l150 \
	rts2-sensor-osn rts2-sensor-system rts2-sensor-apcups rts2-sensor-nut \
	rts2-sensor-cloud2 rts2-sensor-cloud4 rts2-sensor-brooks-356 rts2-sensor-bootes1 rts2-sensor-aag rts2-sensor-pwc \
	rts2-sensor-hygrowin rts2-sensor-fram rts2-sensor-flwo-weather rts2-sensor-flwo-me rts2-sensor-thorlaser rts2-sensor-thorlabs-PM100D \
	rts2-sensor-arduino rts2-sensor-mdm-bait rts2-sensor-colores rts2-sensor-micropirani-925 rts2-sensor-dws-t5 \
	rts2-sensor-cryocon rts2-sensor-keithley-scpi rts2-sensor-colamp \
	rts2-sensor-keithley-487 rts2-sensor-lakeshore-325 rts2-sensor-triax \
	rts2-sensor-mm2500 rts2-sensor-lakeshore-330 rts2-sensor-lakeshore-340 \
	rts2-sensor-agilent-e3631a rts2-sensor-aws-mlab \
	rts2-sensor-thermocube rts2-sensor-reinhardt rts2-sensor-davis \
	rts2-sensor-tbt rts2-sensor-binder \
	rts2-sensor-saao rts2-sensor-cpx400 rts2-sensor-afg rts2-sensor-allnet \
	rts2-sensor-bigelow rts2-sensor-external

noinst_HEADERS = davisudp.h fram-weather.h framudp.h davis.h aag.h pwc-ioctl.h tps534-oak.h

EXTRA_DIST =

LDADD = -L../../lib/rts2 -lrts2gpib -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

if MACOSX
AM_CXXFLAGS += -include ../../include/compat/osx/compat.h
endif

rts2_sensor_dummy_SOURCES = dummy.cpp

rts2_sensor_multi_SOURCES = multi.cpp

rts2_sensor_ms257_SOURCES = ms257.cpp

rts2_sensor_ms260_SOURCES = ms260.cpp

rts2_sensor_l150_SOURCES = l150.cpp

rts2_sensor_newport_lamp_SOURCES = newport-lamp.cpp

rts2_sensor_phytron_SOURCES = phytron.cpp

rts2_sensor_ds21_SOURCES = ds21.cpp

rts2_sensor_mrakomer_SOURCES = mrakomer.cpp

rts2_sensor_cloud2_SOURCES = cloud2.cpp

rts2_sensor_cloud4_SOURCES = cloud4.cpp

rts2_sensor_pixy_SOURCES = pixy.cpp

rts2_sensor_davis_udp_SOURCES = davisudpmain.cpp davisudp.cpp

rts2_sensor_fram_weather_SOURCES = fram-weather.cpp framudp.cpp

rts2_sensor_bart_rain_SOURCES = bart-rain.cpp

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

rts2_sensor_cryocon_SOURCES = cryocon.cpp

rts2_sensor_keithley_scpi_SOURCES = keithley-scpi.cpp

rts2_sensor_keithley_487_SOURCES = keithley-487.cpp

rts2_sensor_lakeshore_325_SOURCES = lakeshore-325.cpp

rts2_sensor_lakeshore_330_SOURCES = lakeshore-330.cpp

rts2_sensor_lakeshore_340_SOURCES = lakeshore-340.cpp

rts2_sensor_triax_SOURCES = triax.cpp

rts2_sensor_mm2500_SOURCES = mm2500.cpp

rts2_sensor_agilent_e3631a_SOURCES = agilent-e3631a.cpp

rts2_sensor_aws_mlab_SOURCES = aws-mlab.cpp

rts2_sensor_saao_SOURCES = saao.cpp
rts2_sensor_saao_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc ${LDADD}

rts2_sensor_cpx400_SOURCES = cpx400.cpp

rts2_sensor_afg_SOURCES = afg.cpp

rts2_sensor_allnet_SOURCES = allnet.cpp
rts2_sensor_allnet_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc ${LDADD}

rts2_sensor_bigelow_SOURCES = bigelow.cpp
rts2_sensor_bigelow_LDADD = @LIBCURL@ ${LDADD}
rts2_sensor_bigelow_CXXFLAGS = ${AM_CXXFLAGS} @LIBCURL_CPPFLAGS@

if SUNCYGMAC
EXTRA_DIST += tps534.cpp tps534-oak.c niratir.cpp clarity.cpp
else
bin_PROGRAMS += rts2-sensor-tps534 rts2-sensor-niratir rts2-sensor-clarity

rts2_sensor_tps534_SOURCES = tps534.cpp tps534-oak.c
rts2_sensor_tps534_LDADD =  @LIB_PTHREAD@ ${LDADD}
rts2_sensor_tps534_CFLAGS = @LIB_NOVA@ -I../../include

rts2_sensor_niratir_SOURCES = niratir.cpp

rts2_sensor_clarity_SOURCES = clarity.cpp
endif

rts2_sensor_flwo_weather_SOURCES = flwo-weather.cpp

rts2_sensor_flwo_me_SOURCES = flwo-me.cpp

rts2_sensor_thorlaser_SOURCES = thorlaser.cpp

rts2_sensor_thorlabs_PM100D_SOURCES = thorlabs-PM100D.cpp

rts2_sensor_arduino_SOURCES = arduino.cpp

rts2_sensor_mdm_bait_SOURCES = mdm_bait.cpp

rts2_sensor_colores_SOURCES = colores.cpp

rts2_sensor_colamp_SOURCES = colamp.cpp

rts2_sensor_micropirani_925_SOURCES = micropirani-925.cpp

rts2_sensor_dws_t5_SOURCES = dws_t5.cpp

rts2_sensor_thermocube_SOURCES = thermocube.cpp

rts2_sensor_reinhardt_SOURCES = reinhardt.cpp

rts2_sensor_davis_SOURCES = davis.cpp

rts2_sensor_tbt_SOURCES = tbt.cpp

rts2_sensor_binder_SOURCES = binder.cpp

rts2_sensor_external_SOURCES = external.cpp

if LIBUSB1
bin_PROGRAMS += rts2-sensor-fows
rts2_sensor_fows_SOURCES = fows.cpp
rts2_sensor_fows_LDADD = ${LDADD} @USB1_LIBS@
rts2_sensor_fows_CPPFLAGS = @USB1_CFLAGS@ -I../../include
else
EXTRA_DIST += fows.cpp
endif

if COMEDI
bin_PROGRAMS += rts2-sensor-bootes2 rts2-sensor-lpnhe

rts2_sensor_bootes2_SOURCES = bootes2.cpp
rts2_sensor_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_sensor_bootes2_CPPFLAGS = @INC_COMEDI@ -I../../include

rts2_sensor_lpnhe_SOURCES = lpnhe.cpp
rts2_sensor_lpnhe_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_sensor_lpnhe_CPPFLAGS = @INC_COMEDI@ -I../../include

else
EXTRA_DIST += bootes2.cpp lpnhe.cpp
endif

if GPIBLIB

LDADD += @LIB_GPIB@

endif

if A3200

bin_PROGRAMS += rts2-sensor-a3200

rts2_sensor_a3200_SOURCES = a3200.cpp
rts2_sensor_a3200_CXXFLAGS = @INC_A3200@ -I../../include
rts2_sensor_a3200_LDADD = @LIB_A3200@ ${LDADD}

else

EXTRA_DIST += a3200.cpp

endif

if BOLTWOOD

bin_PROGRAMS += rts2-sensor-bwcloudsensorii-weather

rts2_sensor_bwcloudsensorii_weather_SOURCES = boltwood.cpp
rts2_sensor_bwcloudsensorii_weather_CXXFLAGS = @INC_BOLTWOOD@ -I../../include
rts2_sensor_bwcloudsensorii_weather_LDADD = @LIB_BOLTWOOD@ ${LDADD}

else

EXTRA_DIST += boltwood.cpp

endif

if BIGNG

bin_PROGRAMS += rts2-sensor-bigng

rts2_sensor_bigng_SOURCES = bigng.cpp
rts2_sensor_bigng_CXXFLAGS = @INC_BIGNG@ -I../../include
rts2_sensor_bigng_LDADD = @LIB_BIGNG@ ${LDADD}

else

EXTRA_DIST += bigng.cpp

endif
