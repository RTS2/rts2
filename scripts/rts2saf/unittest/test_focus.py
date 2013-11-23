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

import unittest

from rts2saf.config  import Configuration 
from rts2saf.environ  import Environment 
from rts2saf.devices import Filter, FilterWheel, Focuser, CCD

from rts2saf.createdevices import CreateFilters, CreateFilterWheels, CreateFocuser, CreateCCD
from rts2saf.checkdevices import CheckDevices
from rts2saf.focus import Focus
from rts2.json import JSONProxy

import subprocess
import os
import sys
import pwd
import grp
import signal
import logging
import time

logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite_focus():
    suite = unittest.TestSuite()
    suite.addTest(TestFocus('test_proxyConnection'))
    suite.addTest(TestFocus('test_focus'))
    return suite

class Args(object):
    def __init__(self):
        pass


class TestFocus(unittest.TestCase):

    def tearDown(self):
        processes=['rts2-centrald','rts2-xmlrpcd','rts2-focusd-dummy','rts2-filterd-dummy', 'rts2-camd-dummy']
        p = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
        out, err = p.communicate()

        for line in out.splitlines():
            # wildi     7432  0.0  0.1  24692  5192 pts/1    S    17:34   0:01 /usr/local/bin/rts2-centrald 
            itms= line.split()
            exe= itms[10].split('/')[-1]
            if self.uid in itms[0] and exe in processes:
                pid = int(itms[1])
                os.kill(pid, signal.SIGTERM)

    def setUp(self):
        # by name
        self.uid=pwd.getpwuid(os.getuid())[0]
        self.gid= grp.getgrgid(os.getgid())[0]
        # sometimes they are present
        self.tearDown()

        # set up rts2saf
        # read configuration
        self.rt = Configuration(logger=logger)
        self.ev=Environment(debug=False, rt=self.rt,logger=logger)
        self.fileName='./rts2saf-bootes-2-autonomous.cfg'
        self.fileName='./rts2saf-bootes-2.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        # set up RTS2
        # rts2-centrald
        cmd=['/usr/local/bin/rts2-centrald', '--run-as', '{}.{}'.format(self.uid,self.gid), '--local-port', '1617', '--logfile', '/tmp/rts2-debug', '--lock-prefix', '/tmp/']
        self.p_centrald= subprocess.Popen(cmd)

        # rts2-xmlrpcd
        cmd=['/usr/local/bin/rts2-xmlrpcd', '--run-as', '{}.{}'.format(self.uid,self.gid), '--lock-prefix', '/tmp/', '--server', '127.0.0.1:1617', '-p', '9999']
        self.p_xmlrpcd= subprocess.Popen(cmd)

        # rts2-focusd-dummy
        focName=self.rt.cfg['FOCUSER_NAME']
        cmd=['/usr/local/bin/rts2-focusd-dummy', '--run-as', '{}.{}'.format(self.uid,self.gid), '--lock-prefix', '/tmp/', '--server', '127.0.0.1:1617', '-d', focName]
        self.p_focusd_dummy= subprocess.Popen(cmd)

        # rts2-filterd-dummy
        ftwns=list()
        for ftwn in self.rt.cfg['inuse']:
            ftwns.append(ftwn)
            cmd=['/usr/local/bin/rts2-filterd-dummy', '--run-as', '{}.{}'.format(self.uid,self.gid), '--lock-prefix', '/tmp/', '--server', '127.0.0.1:1617', '-d', ftwn ]
            ftnames=str()

            for ftn in self.rt.cfg['FILTER WHEEL DEFINITIONS'][ftwn]:
                ftnames += '{}:'.format(ftn)

            if len(ftnames)>0:
                cmd.append('-F')
                cmd.append(ftnames)

            self.p_filterd_dummy= subprocess.Popen(cmd)
        
        # rts2-camd-dummy
        name=self.rt.cfg['CCD_NAME']
        # '--wheeldev', 'COLWSLT',  '--filter-offsets', '1:2:3:4:5:6:7:8'
        cmd=['/usr/local/bin/rts2-camd-dummy', '--run-as', '{}.{}'.format(self.uid,self.gid), '--lock-prefix', '/tmp/', '--server', '127.0.0.1:1617', '-d', name,  '--focdev', focName]
        for nm in ftwns:
            cmd.append('--wheeldev')
            cmd.append(nm)
            if nm in self.rt.cfg['inuse'][0]:
                cmd.append('--filter-offsets')
                cmd.append('1:2:3:4:5:6:7:8')

        self.p_camd_dummy= subprocess.Popen(cmd)
                

        time.sleep(5)

    def setupDevices(self):
        # setup rts2saf
        # fake arguments
        self.args=Args()
        self.args.blind=False
        self.args.verbose=False
        self.args.check=True
        self.args.fetchOffsets=True
        self.args.exposure= 1.887
        self.args.catalogAnalysis=False
        self.args.Ds9Display=False
        self.args.FitDisplay=False
        # JSON
        self.proxy=JSONProxy(url=self.rt.cfg['URL'],username=self.rt.cfg['USERNAME'],password=self.rt.cfg['PASSWORD'])
        # create Focuser 
        self.foc= CreateFocuser(debug=False, proxy=self.proxy, check=self.args.check, rt=self.rt, logger=logger).create()
        # create filters
        fts=CreateFilters(debug=False, proxy=self.proxy, check=self.args.check, rt=self.rt, logger=logger).create()
        # create filter wheels
        ftws= CreateFilterWheels(debug=False, proxy=self.proxy, check=self.args.check, rt=self.rt, logger=logger, filters=fts, foc=self.foc).create()
        # create ccd
        ccd= CreateCCD(debug=False, proxy=self.proxy, check=self.args.check, rt=self.rt, logger=logger, ftws=ftws, fetchOffsets=self.args.fetchOffsets).create()
        
        cdv= CheckDevices(debug=False, proxy=self.proxy, blind=self.args.blind, verbose=self.args.verbose, ccd=ccd, ftws=ftws, foc=self.foc, logger=logger)
        cdv.summaryDevices()
        # ok evrything is there
        self.scd= Focus(debug=False, proxy=self.proxy, args=self.args, dryFitsFiles=None, ccd=ccd, foc=self.foc, ftws=ftws, rt=self.rt, ev=self.ev, logger=logger)



    #@unittest.skip('feature not yet implemented')
    def test_proxyConnection(self):
        self.setupDevices()
        self.proxy.refresh()
        focPos = int(self.proxy.getSingleValue(self.foc.name,'FOC_POS'))
        self.assertEqual( focPos, 0, 'return value:{}'.format(focPos))

    @unittest.skip('this unittest performs a complete focus run, by default it is disabled')
    def test_focus(self):
        self.setupDevices()
        self.scd.run()

if __name__ == '__main__':

    suiteFocus=suite_focus()
    unittest.TextTestRunner(verbosity=0).run(suiteFocus)
