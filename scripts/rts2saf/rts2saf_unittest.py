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
    '/tmp/rts2saf_focus', 
    '/tmp/rts2saf_log', 
]
unittestFiles = [
    '/tmp/centrald_1617', 
    '/tmp/assoc.lst', 
    '/tmp/COLWFLT', 
    '/tmp/COLWGRS', 
    '/tmp/COLWSLT', 
    '/tmp/XMLRPC', 
    '/tmp/andor3', 
    ]

def sextractor_version():
    if os.path.isfile(sex) and os.access(sex, os.X_OK):
        cmd= [sex, '--version']
        versionStr= subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()
        version = versionStr[0].split(' ')[2].split('.')

        if int(version[0]) >= 2 and int(version[1]) >= 8 and int(version[2]) >= 6:
            return True 
        else:
            print 'rts2saf_unittest.py: SExtractor: {}, has version {}.{}.{}, required is 2.8.6, exiting'.format(sex, version[0], version[1], version[2])
            sys.exit(1)
            
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

    if ret:
        return
    else:
        print 'rts2saf_unittest.py: exiting'
        #sys.exit(1)


def executeUnittest():
    os.chdir(unittestDir)
    cmdL = ['python', '-m', 'unittest', 'discover', '-v', '-s', '.']
    cmd = ' '.join(cmdL)
    print cmd
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    while True:
        ln = proc.stdout.readline()
        if ln != '':
            ln.rstrip()
        else:
            break

    os.chdir('../')


def prepareTgz():
    cmdL = [ 'ls', '-lR', '/tmp/rts2saf_focus', '2>&1', '>',  '/tmp/rts2saf_log/rts2saf_focus.ls' ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()
    cmdL = [ 'find',  '/tmp/rts2saf_focus', '-name', '"*png"', '-exec', 'cp', '{}',  '/tmp/rts2saf_log/', '\;' ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()


if __name__ == '__main__':

    sextractor_version()
    unittestDirectory()
    deleteUnittestOutput()
    executeUnittest()
    prepareTgz()
