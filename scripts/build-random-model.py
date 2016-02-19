#!/usr/bin/env python

import pylab
import numpy as np
import random

polar = pylab.subplot(projection='polar')
az=[]
alt=[]
for al in [25.3,36.2,45.1,54.7,63.2,73.1,82.2]:
	for a in np.arange(0 + al/10.0, 360.0, 36.3 * (np.sin(np.radians(al)))):
		az.append(a+random.random() * 3)
		alt.append(al + random.random() * 2)

alt.append(89.2)
az.append(123.34)

print len(az)

azz = []
altt = []

while len(az) > 1:
	i = int(round(random.random() * len(az)))
	if i == len(az):
		i = len(az) - 1
	a = az[i]
	al = alt[i]
	azz.append(a)
	altt.append(al)
	print "{0},{1}".format(round(az[i],2),round(alt[i],2)),
	az = az[0:i] + az[i+1:]
	alt = alt[0:i] + alt[i+1:]

polar.plot(np.radians(azz), 90 - np.array(altt), 'g')
pylab.show()
