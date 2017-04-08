# (C) 2016, Markus Wildi, wildi.markus@bluewin.ch
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

Basic services for connection to HTTPD

'''

__author__ = 'wildi.markus@bluewin.ch'

import os,sys
import configparser
import psycopg2
import crypt
import pwd
from random import choice
from string import ascii_letters


def create_cfg(httpd_connect_string=None, user_name=None,passwd=None,pth_cfg=None,lg=None):
    if not os.path.isfile(pth_cfg):
        cfgf = open(pth_cfg, 'w')
        cfg = configparser.ConfigParser()
        cfg.add_section('proxy')
        cfg.set('proxy', 'host', httpd_connect_string)
        cfg.set('proxy', 'user', user_name)
        cfg.set('proxy', 'passwd', passwd)
        cfg.write(cfgf)
        cfgf.close()
        os.chmod(pth_cfg,0o600)
    else:
        lg.warn('not over writing: {}'.format(pth_cfg))

def delete_cfg(pth_cfg=None,lg=None):
    try:
        os.unlink(pth_cfg) 
    except:
        pass
    
def create_pgsql(user_name=None,passwd=None,lg=None):
    db_user_name=pwd.getpwuid(os.getuid())[0]
    db_name='stars'
    prt_slt=''.join(choice(ascii_letters) for i in range(8))
    pwhash = crypt.crypt(passwd, '$6$' + prt_slt)
    conn = psycopg2.connect('dbname={} user={} password={}'.format(db_name, db_user_name, ''))
    crsr = conn.cursor()
    # usr_login|usr_tmp|usr_email|usr_id|usr_execute_permission|usr_passwd|allowed_devices
    try:
        crsr.execute('INSERT INTO users VALUES (\'{}\', null, \'{}\', 2, \'t\', \'{}\', \'C0 T0 HTTPD\');'.format(user_name,'unittest@example.com',pwhash[0:98]))
    except psycopg2.IntegrityError as e:
        lg.error('create_pgsql: user or email address already exists: {}'.format(user_name))
        return
    conn.commit()
    lg.debug('create_pgsql: {}'.format(crsr.statusmessage))
    crsr.close()
    conn.close()

def delete_pgsql(user_name=None,lg=None):
    db_user_name=pwd.getpwuid(os.getuid())[0]
    db_name='stars'
    conn = psycopg2.connect('dbname={} user={} password={}'.format(db_name, db_user_name, ''))
    crsr = conn.cursor()
    crsr.execute('DELETE FROM users WHERE usr_login=\'{}\' ;'.format(user_name))
    result=crsr.rowcount
    conn.commit()
    crsr.close()
    conn.close()
    #lg.debug('delete_pgsql: {}'.format(crsr.statusmessage))
    if result == 1:
        lg.info('user: {} deleted from database'.format(user_name))
    elif result == 0:
        pass
    else:
        lg.warn('delete user: {} manually, result: {}'.format(user_name, result))
