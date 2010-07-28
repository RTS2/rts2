#!/usr/bin/python

import numpy
import pyfits
import sys

from matplotlib.pyplot import *
import matplotlib.pyplot

a_amp0 = numpy.array ([])
a_amp1 = numpy.array ([])

fig = matplotlib.pyplot.figure()

p1 = fig.add_subplot(221,title='AMP0', xlabel='time (s)', ylabel='pA')
p2 = fig.add_subplot(222, title='AMP1', xlabel='time (s)')
h1 = fig.add_subplot(223)
h2 = fig.add_subplot(224)

for arg in sys.argv[1:]:
	print arg,
	try:
		hdulist = pyfits.open(arg)
		amp0 = hdulist['AMP0.MEAS_TIMES'].data
		amp1 = hdulist['AMP1.MEAS_TIMES'].data

		a_amp0 = numpy.concatenate((a_amp0, amp0.field(1)))
		a_amp1 = numpy.concatenate((a_amp1, amp1.field(1)))

		p1.plot(amp0.field(0),amp0.field(1))
		p2.plot(amp1.field(0),amp1.field(1))
		print ' ok'
	except Exception as ex:
		print ' failed ', ex

p1.grid(True)
p2.grid(True)

h1.hist(a_amp0, 1000)

h2.hist(a_amp1, 1000)
matplotlib.pyplot.show()
