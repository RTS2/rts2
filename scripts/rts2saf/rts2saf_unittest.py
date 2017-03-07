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


import os
import sys
import subprocess
import shutil

sex = '/usr/local/bin/sex'
unittestDir = './unittest'

unittestDirectories = [
    '/tmp/rts2saf_log', 
    '/tmp/rts2saf_focus', 
]
unittestFiles = [
    '/tmp/centrald_1617', 
    '/tmp/assoc.lst', 
    '/tmp/COLWFLT', 
    '/tmp/COLWGRS', 
    '/tmp/COLWSLT', 
    '/tmp/HTTPD', 
    '/tmp/andor3', 
    ]

def userInConfigFile():
    exit = False
    with open('./unittest/rts2saf-bootes-2-autonomous.cfg', 'r') as cfg:
        lines = cfg.readlines()

        global sex
        for ln in lines:
            if '#' in ln:
                continue

            if '=' not in ln:
                continue
            k, v = ln.strip().split('=')
            if 'RTS2_HTTPD_USERNAME' in k and 'YOUR_RTS2_HTTPD_USERNAME' in v:
                exit = True
                print 'RTS2_HTTPD_USERNAME needs a real user as argument'
                
            if 'PASSWORD' in k and 'YOUR_PASSWD' in v:
                exit = True
                print 'PASSWORD needs a real password as argument'
            if 'SEXPATH' in k:
                sex=v
    cfg.close()

    if exit:
        print 'edit ./unittest/rts2saf-bootes-2-autonomous.cfg add your YOUR_RTS2_HTTPD_USERNAME username/password, see documentation section installation, rts2saf unittest'
        sys.exit(1)

def sextractor_version():
    if os.path.isfile(sex) and os.access(sex, os.X_OK):
        cmd= [sex, '--version']
        versionStr= subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()
        version = versionStr[0].split(' ')[2].split('.')
        vs= int(version[0]) * 1000 + int(version[1]) * 100 + int(version[2]) * 10
        vsm = 2 * 1000 + 8 * 100 + 6 * 10
        if vs >  vsm:
            return True 
        else:
            print 'rts2saf_unittest.py: SExtractor: {}, has version {}.{}.{}, required is 2.8.6, exiting'.format(sex, version[0], version[1], version[2])
            #sys.exit(1)
            
    else:
        print 'rts2saf_unittest.py: SExtractor: {}, is not present or not executable, exiting'.format(sex)
        sys.exit(1)

def unittestDirectory():
    if os.path.isdir(unittestDir):
        return True
    else:
        print 'rts2saf_unittest.py: {} not found in local directory, change to ~/rts-2/scripts/rts2saf, exiting'.format(unittestDir)

def deleteUnittestOutput():

    ret = True
    for dr in unittestDirectories:
        if os.path.isdir(dr):
            try:
                shutil.rmtree(dr)
            except Exception as e:
                print e
                print 'rts2saf_unittest.py: can not remove directory: {}, remove it manually'.format(dr)
                ret = False

    for fn in unittestFiles:
        if os.path.exists(fn):
            try:
                os.unlink(fn)
            except Exception as e:
                print e
                print 'rts2saf_unittest.py: can not remove file: {}, remove it manually'.format(fn)
                ret = False
    # make sure that it is writabel (ssh user@host)
    os.makedirs(unittestDirectories[0], mode=0777)

    if ret:
        return
    else:
        print 'rts2saf_unittest.py: exiting'
        #sys.exit(1)


def executeUnittest():
    os.chdir(unittestDir)
#    cmdL = ['python', '-m', 'unittest', 'discover', '-v', '-s', '.', '-p', 'test_fitdisplay.py']
#    cmdL = ['python', '-m', 'unittest', 'discover', '-v', '-s', '.', '-p', 'test_executables.py']
    cmdL = ['python', '-m', 'unittest', 'discover', '-v', '-s', '.']
    cmd = ' '.join(cmdL)
    #
    # hm, where to does this process write to?
    subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).wait()
    os.chdir('../')


def prepareTgz():
    cmdL = [ 'ls', '-lR', '/tmp/rts2saf_focus', '2>&1', '>',  '/tmp/rts2saf_log/rts2saf_focus.ls' ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()
    cmdL = [ 'find',  '/tmp/rts2saf_focus', '-name', '"*png"', '-exec', 'cp', '{}',  '/tmp/rts2saf_log/', '\;' ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()


if __name__ == '__main__':

    userInConfigFile()
    sextractor_version()
    unittestDirectory()
    deleteUnittestOutput()
    executeUnittest()
    prepareTgz()
    print 'DONE'
