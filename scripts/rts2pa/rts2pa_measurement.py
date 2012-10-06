#!/usr/bin/python
# (C) 2004-2012, Markus Wildi, wildi.markus@bluewin.ch
#
#   Measure the position of the our axis based on E.S. King, A.A. Rambaut
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

__author__ = 'wildi.markus@bluewin.ch'

import os
import sys
import rts2pa
import ephem
from datetime import datetime
import threading
import Queue
import math
import time
import rts2.scriptcomm
r2c= rts2.scriptcomm.Rts2Comm()
import pyfits
import rts2.astrometry

class KingA():
    """Calculate the HA, lambda based on E.S. King's method """
    def __init__(self, results=None): # results is a list of SolverResult
        self.results= results
        obs=ephem.Observer()
        obs.lon= str(self.results[0].lon) 
        obs.lat= str(self.results[0].lat)
        #2001-09-30T20:04:21.0
        #to
        #year/month/day optionally followed by hours:minutes:seconds
        obs.date= results[0].date_obs.replace('-', '/').replace('T',' ').replace('.0','')
        siderealT= obs.sidereal_time() 
        self.tau= siderealT - math.radians(15. * results[0].ra)

        self.dY= math.radians(15. *(results[-1].ra - results[0].ra)) * math.cos(math.radians((results[-1].dec+ results[0].dec)/2.)) # astrometry.py result in RA is HH.frac
        self.dX= math.radians(results[-1].dec - results[0].dec)
        self.omega_sid= 2. *  math.pi / 86164.2
        self.dtau= (results[-1].jd- results[0].jd) * self.omega_sid * 86400
        self.success=True
        self.lambda_r=None
        try:
            self.lambda_r= math.sqrt( math.pow(self.dX,2) + math.pow(self.dY,2))/ self.dtau 
        except:
            self.success=False

        self.ha= (-math.atan2( -self.dX, -self.dY) + self.dtau/2. + self.tau) % (2 * math.pi)
        try: # no one observes at +/- 90. deg
            self.A= self.lambda_r * math.sin( self.ha) / math.cos( math.radians(self.results[0].lat)) 
        except:
            self.success=False
        if self.lambda_r:
            self.k= self.lambda_r * math.cos( self.ha)

class SolverResult():
    """Results of astrometry.net including necessary fits headers"""
    def __init__(self, ra=None, dec=None, jd=None, date_obs=None, lon=None, lat=None, fn=None):
        self.ra= ra
        self.dec=dec
        self.jd=jd
        self.date_obs=date_obs
        self.lon=lon
        self.lat=lat
        self.fn= fn

class SolveField():
    """Solve a field with astrometry.net """
    def __init__(self, fn=None, rtc=None, logger=None):
        self.success= True
        self.blind= False
        self.ra= None
        self.dec=None
        self.jd= None
        self.date_obs=None
        self.fn= fn
        self.rtc= rtc
        self.logger = logger
        self.scale  = self.rtc.cp.getfloat('ccd', 'ARCSEC_PER_PIX') 
        self.radius = self.rtc.cp.getfloat('astrometry', 'RADIUS')
        self.replace= self.rtc.cp.getboolean('astrometry', 'REPLACE')
        self.verbose= self.rtc.cp.getboolean('astrometry', 'VERBOSE')
        self.solver = rts2.astrometry.AstrometryScript(self.fn)

        try:
            ff=pyfits.open(self.fn,'readonly')
        except:
            self.logger.error('SolveField: file not found {0}'.format( fn)) 
            self.success= False            

        try:
            self.ra=ff[0].header[self.rtc.cp.get('fits-header','ORIRA')]
            self.dec=ff[0].header[self.rtc.cp.get('fits-header','ORIDEC')]
        except KeyError:
            self.logger.error('SolveField: coordinates key error {0} or {1}, solving blindly'.format( 'ORIRA', 'ORIDEC')) 
            self.blind= True

        try:
            self.jd=ff[0].header[self.rtc.cp.get('fits-header','JD')]
            self.date_obs=ff[0].header[self.rtc.cp.get('fits-header','DATE-OBS')]
        except KeyError:
            self.logger.error('SolveField: date time key error {0} or {1}'.format(self.rtc.cp.get('fits-header','JD'), self.rtc.cp.get('fits-header','DATE-OBS'))) 
            self.success=False

        try:
            self.lon=ff[0].header[self.rtc.cp.get('fits-header','SITE-LON')]
            self.lat=ff[0].header[self.rtc.cp.get('fits-header','SITE-LAT')]
        except KeyError:
            self.logger.error('SolveField: site coordinates key error {0} or {1}'.format(self.rtc.cp.get('fits-header','SITE-LON'), self.rtc.cp.get('fits-header','SITE-LAT'))) 
            self.success=False

        ff.close()

    def solveField(self):
        if self.blind:
            center=self.solver.run(scale=self.scale, replace=self.replace,verbose=self.verbose)
        else:
            center=self.solver.run(scale=self.scale,ra=self.ra,dec=self.dec,radius=self.radius,replace=self.replace,verbose=self.verbose)

        if center!=None:
            if len(center)==2:
#                self.logger.debug('SolveField: found center {0} {1} H.d, D'.format( ephem.degrees(center[0]), ephem.degrees(center[1])))
                self.logger.debug('SolveField: found center {0} {1} H.d, D'.format( center[0], center[1]))
                return SolverResult( ra=center[0], dec=center[1], jd=self.jd, date_obs=self.date_obs, lon=self.lon, lat=self.lat, fn=self.fn)
            else:
                self.logger.debug('SolveField: center not found')
                return None
        else:
            self.logger.debug('SolveField: astrometry.py died badly for file {0}, check solve-field directly if it can find the center'.format(self.fn))
            return None

class MeasurementThread(threading.Thread):
    """Thread receives image path from rts2 and calculates the HA and polar distance of the HA axis intersection as soon as two images are present"""
    def __init__(self, path_q=None, result_q=None, rtc=None, logger=None):
        super(MeasurementThread, self).__init__()
        self.path_q = path_q
        self.result_q = result_q
        self.rtc= rtc
        self.logger= logger
        self.stoprequest = threading.Event()
        self.logger.debug('MeasurementThread: init finished')
        self.results=[]

    def run(self):
        self.logger.debug('MeasurementThread: running')
        if self.rtc.cp.getboolean('basic','TEST'):
            print 'run test'
            # expected are a list of comma separated fits pathes
            # ascending in time
            testPaths= self.rtc.cp.get('basic','TEST_FIELDS').split(',')
            self.logger.debug('MeasurementThread: test replacements fits: {0}'.format(testPaths))

        while not self.stoprequest.isSet():
            path=None
                
            try:
                path = self.path_q.get(True, 1.)            
            except Queue.Empty:
                continue

            if self.rtc.cp.getboolean('basic','TEST'):
                try:
                    path= testPaths.pop(0)
                except:
                    path= None

            self.logger.debug('MeasurementThread: next path {0}'.format(path))
            if path:
                if os.path.isfile(  path):

                    sf= SolveField(fn=path, rtc=self.rtc, logger=self.logger)
                    if sf.success:
                        sr= sf.solveField()
                        if sr:
                            self.results.append(sr)
                            self.result_q.put(self.results)

                            if len(self.results) > 1:
                                kinga=KingA(self.results)
                                self.logger.debug('MeasurementThread: fits {0}'.format(path))      
                                self.logger.debug('MeasurementThread: KingA dx={0} dy={1} arcsec'.format(math.degrees(kinga.dX) *3600., math.degrees(kinga.dY) *3600.))      
                                self.logger.debug('MeasurementThread: KingA dtau={0} arcsec RA= {1} HA={2}'.format( kinga.dtau,sr.ra,math.degrees(kinga.tau)))
                                self.logger.debug('MeasurementThread: KingA HA={0} deg lambda={1} arcsec A={2}, k={3} arcsec'.format( math.degrees(kinga.ha), math.degrees(kinga.lambda_r)*3600,math.degrees(kinga.A) *3600., math.degrees(kinga.k) * 3600.))
                        else:
                            self.result_q.put('MeasurementThread: error within solver (not solving)') # ToDo avoid waiting on results
                            self.logger.error('MeasurementThread: error within solver (not solving)')
                    else:
                            self.result_q.put('MeasurementThread: error within solver') # ToDo avoid waiting on results
                            self.logger.error('MeasurementThread: error within solver')
                else:
                    self.result_q.put('MeasurementThread: {0} does not exist'.format( path)) # ToDo avoid waiting on results
                    self.logger.error('MeasurementThread: {0} does not exist'.format( path))

    def join(self, timeout=None):
        self.logger.debug('MeasurementThread: join, timeout {0}, stopping thread on request'.format(timeout))
        self.stoprequest.set()
        super(MeasurementThread, self).join(timeout)


class AcquireData(rts2pa.PAScript):
    """Create a thread, set the mount, take images"""
    # __init__ of base class is executed

    def takeImages(self, path_q=None, ul=None):
        """Take images and pass them to  MeasurementThread"""
        r2c.setValue('exposure', self.rtc.cp.getfloat('data_taking','EXPOSURE_TIME'))
        r2c.setValue('SHUTTER','LIGHT')

        for img in range(0, ul):
            path = r2c.exposure()
            
            dst= self.env.moveToRunTimePath(path)
            path_q.put(dst)
            self.logger.debug('takeImages: destination image path {0}'.format(dst))
            if not self.rtc.cp.getboolean('basic','TEST'):
                self.logger.debug('takeImages sleeping for {0} seconds'.format(self.rtc.cp.getfloat('data_taking', 'SLEEP')))
                time.sleep(self.rtc.cp.getfloat('data_taking','SLEEP'))

    def setMount(self):
        """Set the mount to the configured location near the celestial pole"""
        self.logger.debug( 'setMount: Starting {0}'.format(self.rtc.cp.get('basic', 'CONFIGURATION_FILE')))
        
        # fetch site latitude
        obs=ephem.Observer()
        obs.lon= str(r2c.getValueFloat('longitude', 'centrald'))
        obs.lat= str(r2c.getValueFloat('latitude', 'centrald'))
        # ha, pd to RA, DEC
        dec= 90. - self.rtc.cp.getfloat('coordinates','PD') 
        siderealT= obs.sidereal_time() 
        ra=  siderealT - math.radians(self.rtc.cp.getfloat('coordinates','HA'))
        # set mount
        if not self.rtc.cp.getboolean('basic', 'TEST'):
            r2c.radec( math.degrees(ra), dec)
            self.logger.debug('longitude: {0}, latitude {1}, sid time {2} ra {3} dec {4}'.format(obs.lon, obs.lat, siderealT, ra, dec))

    def run(self):
        """Set up thread MeasurementThread and start image tacking"""

        self.setMount()
        path_q = Queue.Queue()
        result_q = Queue.Queue()
        mt= MeasurementThread( path_q, result_q, self.rtc, self.logger) 
        mt.start()

        try:
            ul= 1+ int(self.rtc.cp.getfloat('data_taking','DURATION')/self.rtc.cp.getfloat('data_taking','SLEEP'))
            self.logger.debug('run: steps {0}'.format(ul))
            self.logger.info('run: the measurement lasts for {0} seconds and {1} images will be taken'.format(self.rtc.cp.getfloat('data_taking', 'DURATION'),  ul))
        except:
            self.logger.debug('run: sleep must not be zero: {0}'.format(self.rtc.cp.getfloat('data_taking', 'SLEEP')))

        time.sleep(120)
        self.takeImages(path_q, ul)

        lr=0
        while lr < ul:
            results= result_q.get()
            lr= len(results)

        for sr in results:
            try:
                self.logger.info('run: field center at {0} {1} for fits image {2}'.format(sr.ra, sr.dec, sr.fn))
            except:
                pass # error messages indication that thread has a problem already logged to file
                
        mt.join(1.)


if __name__ == "__main__":

    ac= AcquireData(scriptName=sys.argv[0]).run()

