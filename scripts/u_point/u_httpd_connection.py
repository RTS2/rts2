#!/usr/bin/env python3
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
Store connection url, username and password for u_point/devices.py, class DeviceRts2Httpd

'''

__author__ = 'wildi.markus@bluewin.ch'

import os,sys
import argparse
import logging
from random import choice
from string import ascii_letters
from u_point.httpd_connection import create_cfg,delete_cfg,create_pgsql,delete_pgsql

if __name__ == "__main__":

    parser= argparse.ArgumentParser(prog=sys.argv[0], description='Write,delete HTTPD connect string, YOUR_RTS2_HTTPD_USERNAME and password to cfg file and to RTS2 database')
    # basic options
    parser.add_argument('--level', dest='level', default='WARN', help=': %(default)s, debug level')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--base-path', dest='base_path', action='store', default='./unittest',type=str, help=': %(default)s, directory where config file is stored')
    parser.add_argument('--cfg-name', dest='cfg_name', action='store', default='httpd.cfg',type=str, help=': %(default)s, config file name (do not change)')
    parser.add_argument('--create', dest='create', action='store_true', default=False, help=': %(default)s, create config file and database entry')
    parser.add_argument('--httpd-connect-string', dest='httpd_connect_string', default='http://127.0.0.1:8889',type=str,help=': %(default)s, connect string for RTS2 HTTPD ')
    parser.add_argument('--user-name', dest='user_name', default='unittest',type=str,help=': %(default)s, YOUR_RTS2_HTTPD_USERNAME')
    parser.add_argument('--passwd', dest='passwd', default=None,type=str,help=': %(default)s, YOUR_RTS2_HTTPD_PASSWORD')

    args=parser.parse_args()
  
    if args.toconsole:
        args.level='DEBUG'
    
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

    pth_cfg=os.path.join(args.base_path,args.cfg_name)
    if args.create:
        if args.passwd is None:
            # used for unittests only
            passwd=''.join(choice(ascii_letters) for i in range(8))
            logger.info('random password for unittests generated')
        else:
            passwd=args.passwd
            logger.info('password for a real HTTPD user set')
            
        create_cfg(httpd_connect_string=args.httpd_connect_string,user_name=args.user_name,passwd=passwd,pth_cfg=pth_cfg,lg=logger)
        create_pgsql(user_name=args.user_name,passwd=passwd,lg=logger)
    else:
        delete_pgsql(user_name=args.user_name,lg=logger)
        delete_cfg(pth_cfg=pth_cfg,lg=logger)
