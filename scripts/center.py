#!/usr/bin/env python

import rts2

sc = rts2.Rts2Comm()

sc.setValue('filter','C')

c = rts2.Centering()
# step aside 45 arcmin, do 30 second exposure
c.run (0.75,0.75,30)
