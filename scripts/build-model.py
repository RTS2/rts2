#!/usr/bin/python

import rts2.scriptcomm
import time

s = rts2.scriptcomm.Rts2Comm()

s.setValue('exposure',20)
#sidereal tracking
tel = s.getDeviceByType(rts2.scriptcomm.DEVICE_TELESCOPE)
s.setValue('TRACKING',2,tel)

def point_altaz(alt,az):
	s.altaz(alt,az)
	time.sleep(2)
	s.waitIdle(tel,180)
	s.exposure()

# first run at alt 20 degrees
az_r=range(0,360,30)
for az in az_r:
	point_altaz(20,az)

# second at alt 40 degrees
az_r=range(0,360,50)
for az in az_r:
	point_altaz(40,az)

# third at alt 60 degrees
az_r=range(0,360,60)
for az in az_r:
	point_altaz(60,az)

# zenith
point_altaz(90,0)
