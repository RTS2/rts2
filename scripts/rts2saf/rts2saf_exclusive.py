#!/usr/bin/env python

# (C) 2017, Markus Wildi, wildi.markus@bluewin.ch
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

'''

start rts2asaf exclusively (now)

e.g. in crontab (replace $HOME by full path) 
rts2-scriptexec -d C0 -s ' exe $HOME/rts2/script/rts2saf/rts2saf_start.py '
rts2-scriptexec -d C0 -s ' exe $HOME/rts2/script/rts2saf/rts2saf_stop.py '

'''

__author__ = 'wildi.markus@bluewin.ch'

import os
import sys
import logging
import rts2.scriptcomm
import argparse
import datetime



class Script (rts2.scriptcomm.Rts2Comm):
    def __init__(self,lg=None,tar_id=None):
        rts2.scriptcomm.Rts2Comm.__init__(self)
        self.lg=lg
        self.tar_id=tar_id
        
    def initial_values(self):
	selector_enabled=self.getValue('selector_enabled', 'SEL')
        current=self.getValueFloat('current','EXEC')
        current_name=self.getValue('current_name','EXEC')
        current_type=self.getValue('current_type','EXEC')
        self.lg.info('initial_values: EXEC current: {}, current_name: {}, current_type: {}'.format(current,current_name,current_type))
        # check for GLORIA and GRB, and exit if so
        if 'GLORIA teleoperation' in current_name: # reserved observing time
            self.lg.info('initial_values: there is a ongoing GLORIA teleoperation, exiting')
            sys.exit(1)
        elif 'G' in current_type: # it is a GRB
            self.lg.info('initial_values: there is now a GRB target selected, exiting')
            sys.exit(1)
        else:
            self.lg.info('initial_values: neither GLORIA teleoperation nor GRB target')

        selector_next=self.getValueFloat('selector_next','EXEC')
        self.lg.info('initial_values: EXEC selector_next: {}'.format(selector_next))

        selector_enabled=self.getValueFloat('selector_enabled','SEL')
        self.lg.info('initial_values: SEL selector_enabled: {}'.format(selector_enabled))

    def start_rts2saf(self):
	self.setValue('selector_enabled', '0', 'SEL')
	self.setValue('selector_next', '0', 'EXEC')
	self.sendCommand('now {}'.format(self.tar_id), 'EXEC')
        # ToDo ad hoc ok for B2.
        time.sleep(20.)
	self.sendCommand('now 5', 'EXEC')
        self.lg.info('start_rts2saf: disabled SEL, EXEC and sent "now 5" to EXEC')

    def stop_rts2saf(self):
	self.setValue('selector_enabled', '1', 'SEL')
	self.setValue('selector_next', '1', 'EXEC')
        self.lg.info('stop_rts2saf: enabled SEL')

if __name__ == '__main__':
    
    startTime= datetime.datetime.now()
    # since rts2 can not pass options, any option needs a decent default value
    script=os.path.basename(__file__)
    parser= argparse.ArgumentParser(prog=script, description='start rts2asaf exclusively, use sym links rts2saf_(start|stop).py in case of rts2-scriptexec')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--start', dest='start', action='store_true', default=False, help=': %(default)s, start rts2saf')
    group.add_argument('--stop', dest='stop', action='store_true', default=False, help=': %(default)s, stop rts2saf')
    parser.add_argument('--tar-id', dest='tar_id', action='store', default=539, help=': %(default)s, set mount to tar_id=xxx, see your postgres database')

    args=parser.parse_args()

    
    if args.toconsole:
        args.level='DEBUG'
        
    script=os.path.split(sys.argv[0].replace('.py',''))[-1]
    
    filename='/tmp/rts2saf_exclusive.log'   # ToDo datetime, name of the script
    logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
    logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
    logger = logging.getLogger()
    
    if args.toconsole:
        # http://www.mglerner.com/blog/?p=8
        soh = logging.StreamHandler(sys.stdout)
        soh.setLevel(args.level)
        soh.setLevel(args.level)
        logger.addHandler(soh)

        
    sc=Script(lg=logger,tar_id=args.tar_id)
    sc.initial_values()

    if 'start' in script or args.start:
        logger.info('rts2saf_exclusiv: enable rts2saf')
        sc.start_rts2saf()
    elif 'stop' in script or args.stop:
        logger.info('rts2saf_exclusiv: disable rts2saf')
        sc.stop_rts2saf()
    else:
        logger.warn('no argument given, doing nothing')
        
    logger.info('rts2saf_exclusive: DONE')
