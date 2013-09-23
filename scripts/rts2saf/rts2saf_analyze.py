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
try:
    import lib.sextract as sx
except:
    import sextract as sx

try:
    import lib.analyze as anr
except:
    import analyze as anr



import fnmatch
import os
import re
import sys
images = ['*.fits']

# thanks http://stackoverflow.com/questions/635483/what-is-the-best-way-to-implement-nested-dictionaries-in-python
class AutoVivification(dict):
    """Implementation of perl's autovivification feature."""
    def __getitem__(self, item):
        try:
            return dict.__getitem__(self, item)
        except KeyError:
            value = self[item] = type(self)()
            return value


#2013-09-04T22:18:35.077526/fliterwheel/filter
filterWheelWithFilters= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)/(.+?)')
#2013-09-04T22:18:35.077526/(fliterwheel|filter)/
filters= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)/(.+?)')
#2013-09-04T22:18:35.077526/
date= re.compile('.*/([0-9]{4,4}-[0-9]{2,2}-[0-9]{2,2}T[0-9]{2,2}\:[0-9]{2,2}\:[0-9]{2,2}\.[0-9]+)')
#/
plain= re.compile(('.*'))

class Do(object):
    def __init__(self, debug=False, basePath=None, logger=None):
        self.debug=debug
        self.basePath=basePath
        self.logger=logger
        self.ftwPath=AutoVivification()
        self.ftPath=AutoVivification()
        self.datePath=AutoVivification()
        self.plainPath=AutoVivification()


    def __analyzeRun(self, fitsFns=None):
        dataSex=dict()
        for k, fitsFn in enumerate(fitsFns):
            
            logger.info('analyze: processing fits file: {0}'.format(fitsFn))
            rsx= sx.Sextract(debug=args.debugSex, rt=rt, logger=logger)
            dataSex[k]=rsx.sextract(fitsFn=fitsFn) 


        an=anr.Analyze(debug=args.debug, dataSex=dataSex, displayDs9=args.displayDs9, displayFit=args.displayFit, ev=ev, logger=logger)
        weightedMeanObjects, weightedMeanFwhm, minFwhmPos, fwhm= an.analyze()

        logger.info('analyze: result: {0}, {1}, {2}, {3}'.format(weightedMeanObjects, weightedMeanFwhm, minFwhmPos, fwhm))

    def analyzeRuns(self):
        for d  in self.ftwPath.keys():
            for fwt in self.ftwPath[d].keys():
                for ft in self.ftwPath[d][fwt].keys():
                    # print Ftw[d][fwt][ft]
                    self.__analyzeRun(fitsFns=self.ftwPath[d][fwt][ft])

        for d  in self.ftPath.keys():
            for ft in self.ftPath.keys():        
                # print Ft[d][ft]

                self.__analyzeRun(fitsFns=self.ftPath[d][ft])

        for d  in self.datePath.keys():
            # print self.datePath[d]
            self.__analyzeRun(fitsFns=self.datePath[d])

        for p  in self.plainPath.keys():
            # print self.plainPath[d]
            self.__analyzeRun(fitsFns=self.plainPath[p])


    def aggregateRuns(self):
        for root, dirnames, filenames in os.walk(self.basePath):
            for extensions in images:
                for filename in fnmatch.filter(filenames, extensions):
                    ftwwft= filterWheelWithFilters.match(root)
                    ft    = filters.match(root)
                    d     = date.match(root)
                    p     = plain.match(root)
                    if ftwwft:
                        try:
                            self.ftwPath[ftwwft.group(1)][ftwwft.group(2)][ftwwft.group(3)].append(os.path.join(root, filename))
                        except:
                    
                            self.ftwPath[ftwwft.group(1)][ftwwft.group(2)][ftwwft.group(3)]=list()
                            self.ftwPath[ftwwft.group(1)][ftwwft.group(2)][ftwwft.group(3)].append(os.path.join(root, filename))
                    elif ft:
                        try:
                            self.ftPath[ft.group(1)][ft.group(2)].append(os.path.join(root, filename))
                        except:
                    
                            self.ftPath[ft.group(1)][ft.group(2)]=list()
                            self.ftPath[ft.group(1)][ft.group(2)].append(os.path.join(root, filename))
                    elif d:
                        try:
                            self.plainPath[d.group(1)].append(os.path.join(root, filename))
                        except:
                    
                            self.plainPath[d.group(1)]=list()
                            self.plainPath[d.group(1)].append(os.path.join(root, filename))
                    elif p:
                        try:
                            self.plainPath['plain'].append(os.path.join(root, filename))
                        except:
                    
                            self.plainPath['plain']=list()
                            self.plainPath['plain'].append(os.path.join(root, filename))




if __name__ == '__main__':

    import argparse
    import sys
    import logging
    import os
    import glob

    try:
        import lib.config as cfgd
    except:
        import config as cfgd

    try:
        import lib.environ as env
    except:
        import environ as env

    try:
        import lib.log as lg
    except:
        import log as lg

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--debugSex', dest='debugSex', action='store_true', default=False, help=': %(default)s,add more output on SExtract')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--logfile',dest='logfile', default='/tmp/{0}.log'.format(sys.argv[0]), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')
    parser.add_argument('--basepath', dest='basePath', action='store', default=None, help=': %(default)s, directory where FITS images from many focus runs are stored')
#ToDo    parser.add_argument('--ds9region', dest='ds9region', action='store_true', default=False, help=': %(default)s, create ds9 region files')
    parser.add_argument('--displayds9', dest='displayDs9', action='store_true', default=False, help=': %(default)s, display fits images and region files')
    parser.add_argument('--displayfit', dest='displayFit', action='store_true', default=False, help=': %(default)s, display fit')

    args=parser.parse_args()

    # logger
    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 
    # config
    rt=cfgd.Configuration(logger=logger)
    rt.readConfiguration(fileName=args.config)
    # environment
    ev=env.Environment(debug=args.debug, rt=rt,logger=logger)
    ev.createAcquisitionBasePath(ftwName=None, ftName=None)


    if not args.basePath:
        parser.print_help()
        sys.exit(1)

    do=Do(debug=args.debug, basePath=args.basePath)
    do.aggregateRuns()
    do.analyzeRuns()

