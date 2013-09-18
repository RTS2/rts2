#!/usr/bin/python
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'markus.wildi@bluewin.ch'

import sys
import os
import glob
import argparse
import Queue

import lib.acquire as acq
import lib.sextract as sx
import lib.config as cfgd
import lib.analyze as  an
import lib.log as  lg
import lib.environ as env
import lib.devices as dev

if __name__ == '__main__':

    # since rts2 can not pass options, any option needs a decent default value
    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--dryfitsfiles', dest='dryfitsfiles', action='store', default=None, help=': %(default)s, directory where a FITS files are stored from a previous focus run')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf-acquire.cfg', help=': %(default)s, configuration file path')

    args=parser.parse_args()
    # logger
    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 
    # read the run time configuration
    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    # get the environment
    ev=env.Environment(debug=args.debug, rt=rt,log=logger)
    logger.info('rts2saf_focus: starting at: {0}'.format(ev.startTime))
    # check the presence of the devices
    cdv= dev.CheckDevices(debug=args.debug, rt=rt, logger=logger)
    if not cdv.camera():
        logger.error('rts2saf_focus: exiting')
        sys.exit(1)
    if not cdv.focuser():
        logger.error('rts2saf_focus:  exiting')
        sys.exit(1)
    if not cdv.filterWheels():
        logger.error('rts2saf_focus:  exiting')
        sys.exit(1)

    dryFitsFiles=None
    if args.dryfitsfiles:
        dryFitsFiles=glob.glob('{0}/{1}'.format(args.dryfitsfiles, rt.cfg['FILE_GLOB']))

        if len(dryFitsFiles)==0:
            logger.error('rts2saf_focus: no FITS files found in:{}'.format(args.dryfitsfiles))
            logger.info('rts2saf_focus: download a sample from wget http://azug.minpet.unibas.ch/~wildi/rts2saf-test-focus-2013-09-14.tgz')
            sys.exit(1)

    # find clear path on all filter wheels
    # assumption: no combination of filters of the different filter wheels
    for ftw in rt.filterWheelsInUse:
        # first find in ftw.filters a slot with no filter (clear path, offset=0)
        # use this slot first, followed by all others
        ftw.filters.sort(key=lambda x: x.OffsetToEmptySlot)
        for ft in ftw.filters:
            if args.debug: logger.debug('rts2saf_focus: filter wheel: {0:5s}, filter:{1:5s} in use'.format(ftw.name, ft.name))
            if ft.OffsetToEmptySlot==0:
                # ft.emptySlot=Null at instanciation
                try:
                    ftw.emptySlots.append(ft)
                except:
                    ftw.emptySlots=list()
                    ftw.emptySlots.append(ft)

                if args.debug: logger.debug('rts2saf_focus: filter wheel: {0:5s}, filter:{1:5s} is an empty slot'.format(ftw.name, ft.name))

        # warn only if two or more ftws are used
        if not ftw.emptySlots:
            if len(rt.filterWheelsInUse) > 0:
                logger.warn('rts2saf_focus: filter wheel: {0:5s}, no empty slot found'.format(ftw.name))

    # loop over filter wheels, their filters and offsets (FOC_TOFF)
    for ftw in rt.filterWheelsInUse:
        if len(ftw.filters) ==1:
            if ftw.filters[0].OffsetToEmptySlot >0:
                logger.warn('rts2saf_focus: filter wheel: {0} has only one slot: {1}, but it is not  empty'.format(ftw.name,ftw.filters[0].name))
            else:
                if args.debug: logger.debug('rts2saf_focus: filter wheel: {0} has only one slot: {1} and is empty'.format(ftw.name,ftw.filters[0].name))
            continue
        
        for ft in ftw.filters:
            if args.debug: logger.debug('rts2saf_focus: start filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
            # 
            dFF=None
            if args.dryfitsfiles:
                dFF=dryFitsFiles[:]
                logger.info('rts2saf_focus: using FITS files from: {}'.format(args.dryfitsfiles))
                
            # acquisition
            acqu_oq = Queue.Queue()
            acqu= acq.Acquire(debug=args.debug, dryFitsFiles=dFF, ftw=ftw, ft=ft, foc=rt.foc, ccd=rt.ccd, filterWheelsInUse=rt.filterWheelsInUse, acqu_oq=acqu_oq, ev=ev, logger=logger)
            # start acquisition thread
            if not acqu.startScan():
                logger.error('rts2saf_focus: exiting')
                sys.exit(1)
            # acquire FITS
            dataSex=dict()
            cnt=0
            for st in ft.offsets:
                while True:
                    try:
                        fitsFn= acqu_oq.get(block=True, timeout=.2)
                    except Queue.Empty:
                        #if args.debug: logger.debug('rts2saf_focus: continue')
                        continue
                    
                    logger.info('rts2saf_focus: got FITS file name: {}'.format(fitsFn))
                    sxtr= sx.Sextract(debug=args.debug,rt=rt,logger=logger)
                    try:
                        dsx=sxtr.sextract(fitsFn=fitsFn)
                    except Exception, e:
                        logger.error('rts2saf_focus: sextractor faild on fits file:\n{0}'.format(fitsFn,e))
                        logger.error('rts2saf_focus: try to continue')
                        break
                    dataSex[cnt]=dsx
                    cnt +=1
                    break
            else:
                if args.debug: logger.debug('rts2saf_focus: got all images')

            acqu.stopScan(timeout=1.)
            # might be in a thread too
            if dryFitsFiles:
                anr= an.Analyze(dataSex=dataSex, displayDs9=True, displayFit=True, ftwName=ftw.name, ftName=ft.name, ev=ev, logger=logger)
            else:
                anr= an.Analyze(dataSex=dataSex, displayDs9=False, displayFit=False, ftwName=ftw.name, ftName=ft.name, ev=ev, logger=logger)

            (weightedMeanObjects, weightedMeanFwhm, minFwhmPos, fwhm)= anr.analyze()

            if weightedMeanObjects:
                logger.info('rts2saf_focus: {0:5.0f}: weightedMeanObjects'.format(weightedMeanObjects))
            if weightedMeanFwhm:
                logger.info('rts2saf_focus: {0:5.0f}: weightedMeanFwhm'.format(weightedMeanFwhm))

            if minFwhmPos:
                logger.info('rts2saf_focus: {0:5.0f}: minFwhmPos'.format(minFwhmPos))
                logger.info('rts2saf_focus: {0:5.0f}: fwhm'.format(fwhm))

                if ft.OffsetToEmptySlot == 0.:
                    # FOC_DEF (is set first)
                    rt.foc.focDef= minFwhmPos
                    if rt.cfg['SET_FOCUS']:
                        acqu.writeFocDef()
                        logger.info('rts2saf_focus: clear path: {0}, set FOC_DEF: {1}'.format(ft.name, int(minFwhmPos)))
                else:
                    logger.debug('rts2saf_focus: filter: {0} has an offset: {1}'.format(ft.name, int(minFwhmPos- rt.foc.focDef)))
                    ft.OffsetToEmptySlot=int(minFwhmPos- rt.foc.focDef)
            else:
                logger.warn('rts2saf_focus: no fitted minimum found')

            if args.debug: logger.debug('rts2saf_focus: end filter wheel: {}, filter:{}'.format(ftw.name, ft.name))
        else:
            # ToDo wait on Petr's answer
            # write the offsets in the correct order
            if rt.cfg['WRITE_FILTER_OFFSETS']:
                acqu.writeOffsets(ftw=ftw)


        if args.debug: logger.debug('rts2saf_focus: end filter wheel: {}'.format(ftw.name))

#
# Write config with new filter offsets
#
