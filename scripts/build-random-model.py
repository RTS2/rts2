#!/usr/bin/env python

import pylab
import numpy as np
import random

polar = pylab.subplot(projection='polar')
azalt=[]
for al in [25.3,36.2,45.1,54.7,63.2,73.1,82.2]:
	for a in np.arange(0 + al/10.0, 360.0, 36.3 * (np.sin(np.radians(al)))):
		azalt.append([a+random.random() * 3, al + random.random() * 2])

azalt.append([89.2,123.34])

print len(azalt)

path = []

while len(azalt) > 0:
	i = int(round(random.random() * len(azalt)))
	if i == len(azalt):
		i = len(azalt) - 1
	path.append(azalt[i])
	print "{0},{1}".format(round(azalt[i][0],2),round(azalt[i][1],2)),
	azalt = azalt[0:i] + azalt[i+1:]

polar.plot(np.radians(map(lambda x:x[0],path)), 90 - np.array(map(lambda x:x[1],path)), 'g')
pylab.show()
