#!/usr/bin/env python

# match telescope on given position

import rts2

sc = rts2.Rts2Comm()
t = sc.getDeviceByType(rts2.scriptcomm.DEVICE_TELESCOPE)

sc.setValue('filter','C')

c = rts2.Centering()
c.run (0,0,30)

sc.sendCommand('syncorr',t)
