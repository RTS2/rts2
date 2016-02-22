#!/usr/bin/env python

import rts2.altazpath
import numpy as np

def plot_altaz(path):
	import pylab
	polar = pylab.subplot(projection='polar')
	polar.plot(np.radians(map(lambda x:x[1],path)), 90 - np.array(map(lambda x:x[0],path)), 'g')
	pylab.show()

path = rts2.altazpath.random_path()

print len(path)
plot_altaz(path)

for x in path:
	print "{0},{1}".format(round(x[0],2),round(x[1],2)),
