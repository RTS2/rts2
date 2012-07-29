#!/usr/bin/python
# (C) 2004-2012, Markus Wildi, wildi.markus@bluewin.ch
#
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

import sys
import rts2pa
import ephem
import threading
import Queue

import rts2.scriptcomm
r2c= rts2.scriptcomm.Rts2Comm()
import pyfits
import rts2.astrometry
import rts2.libnova

class SolveField():
    def __init__(self, fn=None, runTimeConfig=None, logger=None):
        self.fn= fn
        self.runTimeConfig= runTimeConfig
        self.scale  = self.runTimeConfig.cf['ARCSSEC_PER_PIX'] 
        self.radius = self.runTimeConfig.cf['RADIUS']
        self.verbose= self.runTimeConfig.cf['VERBOSE']
        self.replace= self.runTimeConfig.cf['REPLACE']
        self.solver = rts2.astrometry.AstrometryScript(self.fn)
        self.ra= None
        self.dec=None
	ff=pyfits.fitsopen(self.fn,'readonly')
	self.ra=ff[0].header['ORIRA']
	self.dec=ff[0].header['ORIDEC']
        ff.close()


    def solveField(self):
        center=self.solver.run(scale=self.scale,ra=self.ra,dec=self.dec,radius=self.radius,replace=self.replace,verbose=self.verbose)
        self.logger.debug('SolveField: center {0}'.format( repr(center))) 

class MeasurementThread(threading.Thread):
    def __init__(self, path_q=None, result_q=None, runTimeConfig=None, logger=None):
        super(MeasurementThread, self).__init__()
        self.path_q = path_q
        self.result_q = result_q
        self.runTimeConfig= runTimeConfig
        self.logger= logger
        self.stoprequest = threading.Event()
        self.logger.debug('MeasurementThread: init finished')

    def run(self):
        self.logger.debug('MeasurementThread: running')

        while not self.stoprequest.isSet():
            try:
                path = self.path_q.get(True, 0.05)            
                self.result_q.put(('got: {0}'.format(path)))
                self.logger.debug('MeasurementThread: next path {0}'.format(path))
            except Queue.Empty:
                continue

    def join(self, timeout=None):
        self.logger.debug('MeasurementThread: join, timeout {0}'.format(timeout))
        self.stoprequest.set()
        super(MeasurementThread, self).join(timeout)


class AcquireData(rts2pa.PAScript):
    # __init__ of base class is executed

    def takeImages(self, path_q=None, ul=None):
        r2c.setValue('exposure', self.runTimeConfig.cf['EXPOSURE_TIME'])
        r2c.setValue('SHUTTER','LIGHT')

        for img in range(0, ul):
            path = r2c.exposure()
            
            dst= self.environment.moveToRunTimePath(path)
            path_q.put(dst)
            self.logger.debug('takeImages: destination image path {0}'.format(dst))
            #sleep(self.runTimeConfig.cf['SLEEP'])

    def setMount(self):
        self.logger.debug( 'setMount: Starting {0}'.format(self.runTimeConfig.cf['CONFIGURATION_FILE']))
        
        # fetch site latitude
        obs=ephem.Observer()
        obs.lon= str(r2c.getValueFloat('longitude', 'centrald'))
        obs.lat= str(r2c.getValueFloat('latitude', 'centrald'))
        # ha, pd to RA, DEC
        dec= 90. - self.runTimeConfig.cf['PD'] 
        siderealT= obs.sidereal_time() 
        ra=  siderealT - self.runTimeConfig.cf['HA']
        self.logger.debug('longitude: {0}, latitude {1}, sid time {2} ra {3} dec {4}'.format(obs.lon, obs.lat, siderealT, ra, dec))
        # set mount
        r2c.radec( ra, dec)

    def run(self):
        path_q = Queue.Queue()
        result_q = Queue.Queue()
        mt= MeasurementThread( path_q, result_q, self.runTimeConfig, self.logger) 
        mt.start()
        #self.setMount()

        try:
            ul= int(self.runTimeConfig.cf['DURATION']/self.runTimeConfig.cf['SLEEP'])
            self.logger.debug('run: steps {0}'.format(ul))
        except:
            self.logger.debug('run: sleep must not be zero: {0}'.format(self.runTimeConfig.cf['SLEEP']))

        self.takeImages(path_q, ul)

        for i in range (0, ul):
            self.logger.debug('main: {0}'.format(result_q.get()))

        mt.join(1.)


if __name__ == "__main__":
    ac= AcquireData(scriptName=sys.argv[0])
    ac.run()

