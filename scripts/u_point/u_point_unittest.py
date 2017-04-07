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


import os
import sys
import subprocess
import shutil

unittest_dir = './unittest'

unittest_directories = [
    '/tmp/u_point_unittest', 
]
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
        print('u_point_unittest.py: {} not found in local directory, change to ~/rts2/scripts/u_point, exiting'.format(unittest_dir))

def delete_unittest_output():
    ret = True
    for dr in unittest_directories:
        if os.path.isdir(dr):
            try:
                shutil.rmtree(dr)
            except Exception as e:
                print(e)
                print('u_point_unittest.py: can not remove directory: {}, remove it manually'.format(dr))
                ret = False

    for fn in unittest_files:
        if os.path.exists(fn):
            try:
                os.unlink(fn)
            except Exception as e:
                print(e)
                print('u_point_unittest.py: can not remove file: {}, remove it manually'.format(fn))
                ret = False
    # make sure that it is writabel (ssh user@host)
    os.makedirs(unittest_directories[0], mode=0o0777)

    if ret:
        return
    else:
        print('u_point_unittest.py: exiting')
        #sys.exit(1)


def execute_unittest():
    os.chdir(unittest_dir)
    cmdL = ['python', '-m', 'unittest', 'discover', '-v', '-s', '.']
    cmd = ' '.join(cmdL)
    #
    # hm, where to does this process writes to?
    subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).wait()
    os.chdir('../')


def prepare_tgz():
    #ToDo
    cmdL = [ 'ls', '-lR', '/tmp/u_point_unittest', '2>&1', '>',  '/tmp/u_point_unittest/u_point_unittest.ls' ]
    cmd = ' '.join(cmdL)
    proc = subprocess.Popen(cmd, shell=True).wait()
    #cmdL = [ 'find',  '/tmp/u_point_focus', '-name', '"*png"', '-exec', 'cp', '{}',  '/tmp/u_point_log/', '\;' ]
    #cmd = ' '.join(cmdL)
    #proc = subprocess.Popen(cmd, shell=True).wait()


if __name__ == '__main__':

    unittest_directory()
    delete_unittest_output()
    execute_unittest()
    prepare_tgz()
    print('DONE')
