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


import time
import datetime
import glob
import os


class Environment(object):
    """Class performing various task on files, e.g. expansion to (full) path. Provides the start time.

    :var debug: enable more debug output with --debug and --level
    :var rt: run time configuration,  :py:mod:`rts2saf.config.Configuration`, usually read from /usr/local/etc/rts2/rts2saf/rts2saf.cfg
    :var logger:  :py:mod:`rts2saf.log`
    :var startTime: self.startTime= datetime.datetime.now().isoformat()

    """
    def __init__(self, debug=None, rt=None, logger=None):
        self.startTime= datetime.datetime.now().isoformat()
        self.debug=debug
        self.rt=rt
        self.logger=logger

    def expandToTmp( self, fn=None):
        fn= self.rt.cfg['TEMP_DIRECTORY'] + '/'+ fn
        return fn

    def expandToPlotFileName( self, plotFn=None):
        if plotFn==None:
            self.logger.error('Environment.expandToPlotFileName: no file name given')
            plotFn='myPlot.png'

        base, ext= os.path.splitext(plotFn)
        return '{}-{}{}'.format(base,self.startTime[0:19],ext)

    def expandToAcquisitionBasePath(self, ftwName=None, ftName=None):

        if ftwName== 'FAKE_FTW' and ftName=='FAKE_FT':
            return self.rt.cfg['BASE_DIRECTORY'] + '/' + self.startTime + '/'  

        elif ftwName== None and ftName==None:
            return self.rt.cfg['BASE_DIRECTORY'] + '/' + self.startTime + '/'  

        elif ftwName== None:
            return self.rt.cfg['BASE_DIRECTORY'] + '/' + self.startTime + '/'  + ftName + '/'

        elif ftName== None:
            return self.rt.cfg['BASE_DIRECTORY'] + '/' + self.startTime + '/'  + ftwName + '/'
        else: 
            return self.rt.cfg['BASE_DIRECTORY'] + '/' + self.startTime + '/' + ftwName + '/' + ftName + '/'
        
    def createAcquisitionBasePath(self, ftwName=None, ftName=None):
        pth= self.expandToAcquisitionBasePath( ftwName=ftwName, ftName=ftName)
        if not os.path.exists(pth):
            try:
                os.makedirs( pth)
            except Exception, e:
                self.logger.error('Environment.createAcquisitionBasePath failed for {0}, {1}'.format(pth, e))
                return False
        return True
