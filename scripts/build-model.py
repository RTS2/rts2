#!/usr/bin/env python

import rts2.altazpath
import rts2.scriptcomm
import time

s = rts2.scriptcomm.Rts2Comm()

s.setValue('exposure',20)
s.setValue('SHUTTER',0)
#sidereal tracking
tel = s.getDeviceByType(rts2.scriptcomm.DEVICE_TELESCOPE)
s.setValue('TRACKING',2,tel)

def point_altaz(alt,az):
	s.altaz(alt,az)
	time.sleep(2)
	s.waitIdle(tel,180)
	img=s.exposure()
        s.process(img)

for p in rts2.altazpath.random_path():
#for p in rts2.altazpath.constant_path():
	point_altaz(p[0],p[1])
