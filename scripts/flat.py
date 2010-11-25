#!/usr/bin/python

# Example configuation of flats. 
# (C) 2010 Petr Kubanek
#
# Please see flats.py file for details about needed files.
#
# You most probably would like to modify this file to suit your needs.
# Please see comments in flats.py for details of the parameters.

from flats import FlatScript,Flat

# You would at least like to specify filter order, if not binning and other things
f = FlatScript(eveningFlats=[Flat('Z'),Flat('u'),Flat('V'),Flat('B'),Flat('g'),Flat('i'),Flat('r'),Flat('CLR')],doDarks=True,expTimes=range(1,40))

# Change deafult number of images
f.flatLevels(defaultNumberFlats=5,biasLevel=550,allowedOptimalDeviation=0.1)

# Run it..
f.run()

# Send some optional emails - configure before uncomenting this line
# f.sendEmail('robot@example.com','Example skyflats')
