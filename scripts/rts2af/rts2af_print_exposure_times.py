#!/usr/bin/python


import rts2af
import math


def acquireImage():

    
    serviceFileOp= rts2af.serviceFileOp= rts2af.ServiceFileOperations()
    runTimeConfig= rts2af.runTimeConfig= rts2af.Configuration() # default config

    runTimeConfig.readConfiguration('/etc/rts2/rts2af/rts2af-acquire.cfg')

    flt= runTimeConfig.filterByName( 'C')
    base_exposure= runTimeConfig.value('DEFAULT_EXPOSURE')

    focuser= rts2af.Focuser(name='FOC_FLI', lowerLimit=100, upperLimit=7000, resolution=20, speed=90., stepSize=1.1428e-6)
    #telescope= rts2af.Telescope(radius=.2, focalLength=4.)
    telescope= rts2af.Telescope(radius=.09, focalLength=1.26)

    print 'Telescope, CCD, sky  properties: radius: {0}, focalLength: {1}, pixelSize: {2}, seeing: {3} (meter), base exposure time: {4}'.format(telescope.radius, telescope.focalLength, telescope.pixelSize, telescope.seeing, base_exposure)

    # loop over the focuser steps
    i= 0
    sum_normal=0
    sum_adapted=0
    for setting in flt.settings:
        exposure=  telescope.linearExposureTimeAtFocPos(base_exposure * setting.exposureFactor, setting.offset * focuser.stepSize)
        print ('rts2af_test: Linear:    filter {0} offset {1:5.0f} exposure {2}, true exposure: {3:5.1f}'.format(flt.name, setting.offset, base_exposure * setting.exposureFactor, exposure))
        i += 1
        sum_normal += base_exposure * setting.exposureFactor
        sum_adapted+= exposure


    print 'Linear summs {0}, {1}'.format(sum_normal, sum_adapted)

    i= 0
    sum_normal=0
    sum_adapted=0
    for setting in flt.settings:

        exposure=  telescope.quadraticExposureTimeAtFocPos(base_exposure * setting.exposureFactor, setting.offset * focuser.stepSize)
        print ('rts2af_test: Quadratic, filter {0} offset {1:5.0f} exposure {2}, true exposure: {3:5.1f}'.format(flt.name, setting.offset, base_exposure * setting.exposureFactor, exposure))
        i += 1
        sum_normal += base_exposure * setting.exposureFactor
        sum_adapted+= exposure

    print 'Quadratic summs {0}, {1}'.format(sum_normal, sum_adapted)



acquireImage()
