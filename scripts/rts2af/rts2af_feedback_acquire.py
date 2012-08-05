#!/usr/bin/python
# (C) 2012, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_feedback_acquire.py --help
#   
#
#   rts2af_feedback_acquire.py is called by rts2af_acquire.py in test mode
#   during an rts2 initiated focus run. It is not intended for interactive use.
#   
#   rts2af_feedback_acquire.py has no connection via rts2.scriptcomm.py to rts2-centrald
#   Therefore logging must be done do a separate file.
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
# required packages:
# wget 'http://argparse.googlecode.com/files/argparse-1.1.zip

__author__ = 'markus.wildi@one-arcsec.org'

import sys
import re
import os
import logging
import rts2af 

class FeedBack():
    """extract the catalgue of an images"""
    def __init__(self, scriptName=None):
        self.scriptName=scriptName

    def run(self):

        referenceFitsFileName = sys.stdin.readline().strip()

        logging.info('rts2af_feedback_acquire.py: pid: {0}, starting, reference file: {1}'.format(os.getpid(), referenceFitsFileName))

        print 'info: reference catalogue created'
        sys.stdout.flush()

        # read the files sys.stdin.readline() normally  rts2af_feedback_acquire.py is fed by rts2af_acquire.py
        while(True):
                         
            fits=None
            try:
                fits= sys.stdin.readline().strip()
            except:
                logging.info('rts2af_feedback_acquire.py: got EOF, breaking')
                break

            if( fits==None):
                logging.info('rts2af_feedback_acquire.py: got None, breaking')
                break
            
            quit_match= re.search( r'Q', fits)

            if( not quit_match==None): 
                logging.info('rts2af_feedback_acquire.py: got Q, breaking')
                break
            if( len(fits)==0): 
                logging.error('rts2af_feedback_acquire.py: got a zero length input from stdin, exiting')
                sys.exit(1)

            logging.info('rts2af_feedback_acquire.py: got fits file >>{0}<<'.format(fits))

        logging.info('rts2af_feedback_acquire.py: performing fake fit now: {0}'.format(referenceFitsFileName))
                         
        minimumFocPos= 3020.0
        minimumFwhm= 3.5
        temperature= '17.2C'
        objects= 88
        nrDatapoints= 20

        chi2= -1
        dateEpoch= -1
        withinBounds=True
        referenceFileName= referenceFitsFileName
        constants= [0,0,0]

        print 'FOCUS: {0}, FWHM: {1}, TEMPERATURE: {2}, OBJECTS: {3} DATAPOINTS: {4}'.format(minimumFocPos, minimumFwhm, temperature, objects, nrDatapoints)
        logging.info('rts2af_feedback_acquire.py: fit result {0}, reference file: {1}'.format(minimumFocPos, referenceFitsFileName))
        # input format for rts2af_model_analyze.py
        logging.info('rts2af_feedback_acquire.py: {0} {1} {2} {3} {4} {5} {6} {7} {8} {9} {10}\n'.format(chi2, temperature, temperature, objects, minimumFocPos, minimumFwhm, dateEpoch, withinBounds, referenceFileName, nrDatapoints, constants))
        
        logging.info('rts2af_feedback_acquire.py: pid: {0}, ending, reference file: {1}'.format(os.getpid(), referenceFitsFileName))

if __name__ == '__main__':
    FeedBack(sys.argv[0]).run()


