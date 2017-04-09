# (C) 2017, Markus Wildi, markus.wildi@bluewin.ch
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
import subprocess
import os
import sys
import pwd
import grp
import signal
import logging
import time
import glob
import psycopg2

basepath='/tmp/u_point_unittest'
if not os.path.exists(basepath):
    # make sure that it is writabel (ssh user@host)
    ret=os.makedirs(basepath, mode=0o0777)

logging.basicConfig(filename=os.path.join(basepath,'unittest.log'), level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()


class Args(object):
    def __init__(self):
        pass


class RTS2Environment(unittest.TestCase):

    def tearDown(self):
        processes=['rts2-centrald','rts2-executor', 'rts2-httpd','rts2-focusd-dummy','rts2-filterd-dummy', 'rts2-camd-dummy', 'rts2-teld-dummy']
        p = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
        out, err = p.communicate()
        for line in out.decode("utf-8").splitlines():
            # wildi     7432  0.0  0.1  24692  5192 pts/1    S    17:34   0:01 /usr/local/bin/rts2-centrald 
            itms= line.split()
            exe= itms[10].split('/')[-1]
            if self.uid in itms[0] and exe in processes:
                pid = int(itms[1])
                os.kill(pid, signal.SIGTERM)

        # reove th lock files
        for fn in glob.glob(self.lockPrefix+'*'):
            os.unlink (fn)

    def setUp(self):
        # by name
        self.uid=pwd.getpwuid(os.getuid())[0]
        self.gid= grp.getgrgid(os.getgid())[0]
        # lock prefix
        self.lockPrefix = '/tmp/rts2_{}'.format(self.uid)
        # sometimes they are present
        self.tearDown()
        # set up RTS2
        # rts2-centrald
        cmd=[ '/usr/local/bin/rts2-centrald', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--local-port', '1617', 
              '--logfile', os.path.join(basepath,'rts2-debug'), 
              '--lock-prefix', self.lockPrefix, 
              '--config', './rts2-unittest.ini'
        ]
        self.p_centrald= subprocess.Popen(cmd)

        # rts2-executor
        cmd=[ '/usr/local/bin/rts2-executor', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--config', './rts2-unittest.ini',
              '--server', '127.0.0.1:1617', 
              '--noauth'
        ]
        self.p_exec= subprocess.Popen(cmd)

        # rts2-httpd
        cmd=[ '/usr/local/bin/rts2-httpd', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--config', './rts2-unittest.ini',
              '--server', '127.0.0.1:1617', 
              '-p', '9999', 
              '--noauth',# seems not to work
        ]
        self.p_httpd= subprocess.Popen(cmd)

        # rts2-focusd-dummy
        focName='F0'
        cmd=[ '/usr/local/bin/rts2-focusd-dummy', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--server', '127.0.0.1:1617', 
              '-d', focName, 
              '--modefile', './f0.modefile',
              #'--config', './rts2-unittest.ini'
        ]
        self.p_focusd_dummy= subprocess.Popen(cmd)

        # rts2-filterd-dummy
        ftwnName='W0'
        cmd=[ '/usr/local/bin/rts2-filterd-dummy', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--server', '127.0.0.1:1617', 
              '-d', ftwnName, 
              #'--config', './rts2-unittest.ini'
        ]

        ftnames = 'U:B:V:R:I:H:X'
        cmd.append('-F')
        cmd.append(ftnames)

        self.p_filterd_dummy= subprocess.Popen(cmd)
        
        # rts2-camd-dummy
        name='C0'
        # '--wheeldev', 'W0',  '--filter-offsets', '1:2:3:4:5:6:7'
        cmd=[ '/usr/local/bin/rts2-camd-dummy', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--server', '127.0.0.1:1617', 
              '-d', name,  
              '--focdev', focName,
              # not available
              #'--config', './rts2-unittest.ini'
        ]
        cmd.append('--wheeldev')
        cmd.append(ftwnName)
        cmd.append('--filter-offsets')
        cmd.append('1:2:3:4:5:6:7')

        self.p_camd_dummy= subprocess.Popen(cmd)
        
        # rts2-teld-dummy
        mntName='T0'
        cmd=[ '/usr/local/bin/rts2-teld-dummy', 
              '--run-as', '{}.{}'.format(self.uid,self.gid), 
              '--lock-prefix', self.lockPrefix, 
              '--server', '127.0.0.1:1617', 
              '-d', mntName, 
              '--modefile', './t0.modefile',
              #'--config', './rts2-unittest.ini'
        ]
        self.p_teld_dummy= subprocess.Popen(cmd)
        #
        print('sleeping 10 sec')
        time.sleep(10)
        print('sleeping over')

