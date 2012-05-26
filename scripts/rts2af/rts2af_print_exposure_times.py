#!/usr/bin/python


import rts2af
import math


def acquireImage():

    focDef=3000
    focToff=-6000
    OffsetToClearPath= 500

    focSet= focDef + focToff + OffsetToClearPath

    if(focSet > 5500):
        focToff= 5500 -focDef - OffsetToClearPath
        print 'rts2af_acquire.py: greater than upperLimit, setting focToff to: {0}'.format(focToff)
        
    if(focSet < 500):
        focToff= 500 -focDef -OffsetToClearPath
        print 'rts2af_acquire.py: lower than lowerLimit, setting focToff to: {0}'.format(focToff)


serviceFileOp= rts2af.serviceFileOp= rts2af.ServiceFileOperations()
runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config
runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-acquire.cfg') # rts2 exe mechanism has no options


telescope= rts2af.Telescope()
flt= runTimeConfig.filterByName( 'X')

print 'Telescope, CCD, sky  properties: radius: {0}, focalLength: {1}, pixelSize: {2}, seeing: {3} (meter), fudge factor: {4}'.format(telescope.radius, telescope.focalLength, telescope.pixelSize, telescope.seeing, telescope.fudgeFactor )

# loop over the focuser steps
i= 0
sum_normal=0
sum_adapted=0
for setting in flt.settings:
    exposure=  telescope.linearExposureTimeAtFocPos(setting.exposure, setting.offset)
    print ('rts2af_test: Linear: filter {0} offset {1} exposure {2}, true exposure: {3}'.format(flt.name, setting.offset, setting.exposure, exposure))
    i += 1
    sum_normal += setting.exposure
    sum_adapted+= exposure


print 'Linear summs {0}, {1}'.format(sum_normal, sum_adapted)

i= 0
sum_normal=0
sum_adapted=0
for setting in flt.settings:

    
    if(( abs(setting.offset) < 410) or ( abs(setting.offset)==720)):

        exposure=  telescope.quadraticExposureTimeAtFocPos(setting.exposure, setting.offset)
        print ('rts2af_test: Quadratic, filter {0} offset {1} exposure {2}, true exposure: {3}'.format(flt.name, setting.offset, setting.exposure, exposure))
        i += 1
        sum_normal += setting.exposure
        sum_adapted+= exposure
    else:
        sum_normal += setting.exposure


print 'Quadratic summs {0}, {1}'.format(sum_normal, sum_adapted)



acquireImage()


