#!/usr/bin/env python3

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

__author__ = 'wildi.markus@bluewin.ch'

import signal
import os
import sys
import argparse
import logging
import subprocess
import shutil
import crypt
import pwd
from u_point.httpd_connection import create_cfg,delete_cfg,create_pgsql,delete_pgsql
from random import choice
from string import ascii_letters


def receive_signal(signum, stack):
    global logger
    global user_name
    global pth_cfg
    delete_pgsql(user_name=user_name,lg=logger) 
    delete_cfg(pth_cfg=pth_cfg,lg=logger)
    # ToDo does not appear
    logger.warn('receive_signal: received: {}, deleted database user and cfg file'.format(signum))
    sys.exit(1)
    
signal.signal(signal.SIGUSR1, receive_signal)
signal.signal(signal.SIGUSR2, receive_signal)
signal.signal(signal.SIGTERM, receive_signal)
signal.signal(signal.SIGHUP,  receive_signal)
signal.signal(signal.SIGQUIT, receive_signal)
signal.signal(signal.SIGINT,  receive_signal)

unittest_dir = './unittest'

unittest_files = [
    '/tmp/centrald_1617', 
    '/tmp/T0', 
    '/tmp/C0', 
    '/tmp/HTTPD', 
    ]

def unittest_directory():
    if os.path.isdir(unittest_dir):
        return True
    else:
        lg.error('u_point_unittest.py: {} not found in local directory, change to ~/rts2/scripts/u_point, exiting'.format(unittest_dir))
        sys.exit(1)
        
def delete_unittest_output(lg=None):
    ret = True
    for fn in unittest_files:
        if os.path.exists(fn):
            try:
                os.unlink(fn)
            except Exception as e:
                lg.error(e)
                lg.warn('u_point_unittest.py: can not remove file: {}, remove it manually'.format(fn))
                ret = False
    if ret:
        return
    else:
        lg.warn('u_point_unittest.py: exiting')
        sys.exit(1)

def execute_unittest():
    os.chdir(unittest_dir)
    cmdL = ['python3', '-m', 'unittest', 'discover', '-v', '-s', '.']
    cmd = ' '.join(cmdL)
    #
    # hm, where to does this process writes to?
    subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).wait()
    os.chdir('../')


def prepare_tgz(bpt=None):
    #ToDo
    cmdL = [ 'ls', '-lR', bpt, '2>&1', '>',  '{}/u_point_unittest.ls'.format(bpt) ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()
    #cmdL = [ 'find',  '/tmp/u_point_focus', '-name', '"*png"', '-exec', 'cp', '{}',  '/tmp/u_point_log/', '\;' ]
    #cmd = ' '.join(cmdL)
    #proc = subprocess.Popen(cmd, shell=True).wait()


if __name__ == '__main__':
    
    global logger
    global user_name
    global pth_cfg
    parser= argparse.ArgumentParser(prog=sys.argv[0], description='Write HTTPD connect string, YOUR_RTS2_HTTPD_USERNAME and password to cfg file and to RTS2 database')
    # basic options
    parser.add_argument('--level', dest='level', default='DEBUG', help=': %(default)s, debug level')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--do-not-delete', dest='do_not_delete', action='store_true', default=False, help=': %(default)s, do not delete base path')
    parser.add_argument('--base-path', dest='base_path', action='store', default='/tmp/u_point_unittest',type=str, help=': %(default)s, directory where config file is stored')
    args=parser.parse_args()

    if os.path.exists(args.base_path):
        if args.do_not_delete:
            pass
        else:
            print('recreate base path')
            # ok, not unittest like but effective
            try:
                shutil.rmtree(args.base_path)
            except Exception as e:
                print(e)
                print('u_point_unittest.py: can not remove directory: {}, remove it manually'.format(args.base_path))
    
    # make sure that it is writabel (ssh user@host)
    try:
        os.makedirs(args.base_path, mode=0o0777)
    except:
        pass
        
    pth, fn = os.path.split(sys.argv[0])
    filename=os.path.join(args.base_path,'{}.log'.format(fn.replace('.py',''))) # ToDo datetime, name of the script
    logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
    logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
    logger = logging.getLogger()
    
    if args.toconsole:
        # http://www.mglerner.com/blog/?p=8
        soh = logging.StreamHandler(sys.stdout)
        soh.setLevel(args.level)
        soh.setLevel(args.level)
        logger.addHandler(soh)
        
    unittest_directory()
    #delete_unittest_output(lg=logger)
    
    user_name='unittest'
    passwd=''.join(choice(ascii_letters) for i in range(8))
    
    pth_cfg=os.path.join(args.base_path,'httpd.cfg')
    create_cfg(httpd_connect_string='http://127.0.0.1:9999', user_name=user_name,passwd=passwd,pth_cfg=pth_cfg,lg=logger)
    create_pgsql(user_name='unittest',passwd=passwd,lg=logger)
    #
    execute_unittest()
    #
    delete_pgsql(user_name=user_name,lg=logger)
    delete_cfg(pth_cfg=pth_cfg,lg=logger)
    #
    prepare_tgz(bpt=args.base_path)
    logger.info('DONE')
