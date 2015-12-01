#!/usr/bin/python
#
# Calculates median from files suplied as arguments. Median
# is saved to file specified as first argument.
#
# You will need numpy and pyfits packages. Those are available from python-numpy
# and python-pyfits Debian/Ubuntu packages.
#
# Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>

import sys
import numpy
import pyfits
import os

f1 = pyfits.open(sys.argv[1])
header = f1[0].header

bias = pyfits.open(sys.argv[2])

substract = f1[0].data - bias[0].data

of = pyfits.open(sys.argv[3],mode='append')
i = pyfits.PrimaryHDU(data=substract,header=f1[0].header)
of.append(i)
of.close()

f1.close()
bias.close()
