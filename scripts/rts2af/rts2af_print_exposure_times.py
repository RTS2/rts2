#!/usr/bin/python

import sys
import rts2af
import math


def CalculateExposureTimes():

    
    rtc= rts2af.Configuration('/etc/rts2/rts2af/rts2af-acquire.cfg')
    env= rts2af.Environment( rtc=rtc, log=None)

    filter= rtc.filterByName( 'X')
    if not filter:
        print 'no filter config found for {0}'.format('C')
    base_exposure= rtc.value('DEFAULT_EXPOSURE')

    focuser= rts2af.Focuser(name='FOC_FLI', lowerLimit=100, upperLimit=7000, resolution=20, speed=90., stepSize=1.1428e-6)
    #telescope= rts2af.Telescope(radius=.2, focalLength=4.)
    telescope= rts2af.Telescope(radius=env.rtc.value('TEL_RADIUS'), focalLength=env.rtc.value('TEL_FOCALLENGTH'), pixelSize=env.rtc.value('PIXELSIZE'), seeing=env.rtc.value('SEEING'))

    print 'Telescope, CCD, sky  properties: radius: {0}, focalLength: {1}, pixelSize: {2}, seeing: {3} (meter), base exposure time: {4}'.format(telescope.radius, telescope.focalLength, telescope.pixelSize, telescope.seeing, base_exposure)

    # loop over the focuser steps
    i= 0
    sum_normal=0
    sum_adapted=0
    for offset in filter.offsets:
        exposure=  telescope.linearExposureTimeAtFocPos(base_exposure * filter.exposureFactor, offset * focuser.stepSize)
        print ('rts2af_test: Linear:    filter {0} offset {1:5.0f} exposure {2}, true exposure: {3:5.1f}'.format(filter.name, offset, base_exposure * filter.exposureFactor, exposure))
        i += 1
        sum_normal += base_exposure * filter.exposureFactor
        sum_adapted+= exposure


    print 'Linear summs {0}, {1}'.format(sum_normal, sum_adapted)

    i= 0
    sum_normal=0
    sum_adapted=0
    for offset in filter.offsets:

        exposure=  telescope.quadraticExposureTimeAtFocPos(base_exposure * filter.exposureFactor, offset * focuser.stepSize)
        print ('rts2af_test: Quadratic, filter {0} offset {1:5.0f} exposure {2}, true exposure: {3:5.1f}'.format(filter.name, offset, base_exposure * filter.exposureFactor, exposure))
        i += 1
        sum_normal += base_exposure * filter.exposureFactor
        sum_adapted+= exposure

    print 'Quadratic summs {0}, {1}'.format(sum_normal, sum_adapted)


if __name__ == '__main__':

    CalculateExposureTimes()
