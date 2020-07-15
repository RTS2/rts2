#!/usr/bin/env python

# Example configuation of flats.
# (C) 2010 Petr Kubanek
#
# Please see flats.py file for details about needed files.
#
# You most probably would like to modify this file to suit your needs.
# Please see comments in flats.py for details of the parameters.

from rts2.flats import FlatScript, Flat
import rts2.scriptcomm

# You would at least like to specify filter order, if not binning and
# other things
f = FlatScript(
    eveningFlats=[
        Flat('i', binning=1, window='100 100 500 500', expTimes=[5, 10, 40])
    ],
    maxBias=1, maxDarks=0, expTimes=range(1, 40)
)

# Change deafult number of images
f.flatLevels(defaultNumberFlats=3, biasLevel=550, allowedOptimalDeviation=0.1)

# Run it..
# Configure domeDevice,tmpDirectory and mountDevice if your device names differ
f.run()

# Or send some optional emails - configure scripting before uncomenting this
# line
# f.run(receivers='robot@example.com',subject='Example skyflats')
