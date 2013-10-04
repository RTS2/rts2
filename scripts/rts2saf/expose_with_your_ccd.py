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

__author__ = 'markus.wildi@bluewin.ch'

import subprocess
import os
import errno
import argparse
import re
import sys
import shutil
# 
# thanks to: http://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
#
from functools import wraps
import errno
import os
import signal

class TimeoutError(Exception):
    pass

def timeout(seconds=10, error_message=os.strerror(errno.ETIME)):
    def decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        def wrapper(*args, **kwargs):
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.alarm(seconds)
            try:
                result = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return result

        return wraps(func)(wrapper)

    return decorator

@timeout(seconds=10, error_message=os.strerror(errno.ETIMEDOUT))
def method1(debug=False, ccdName=None):
    exp= 0.001
    cmd = [ '/usr/local/bin/rts2-scriptexec' , '-d', ccdName, '-s', "' D {0} '".format(exp)]
    proc  = subprocess.Popen( cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_value, stderr_value = proc.communicate()
    print repr(stdout_value)
    print repr(stderr_value)
    return stdout_value

@timeout(seconds=10, error_message=os.strerror(errno.ETIMEDOUT))
def method2(debug=False, ccdName=None):
    exp= 0.001
    cmd = [ '/usr/local/bin/rts2-scriptexec' , '-d', ccdName, '-s', "' D {0} '".format(exp)]
    return subprocess.check_output( cmd)


@timeout(seconds=10, error_message=os.strerror(errno.ETIMEDOUT))
def method3(debug=False, ccdName=None):
    exp= 0.001
    cmd = [ '/bin/bash' ]
    proc  = subprocess.Popen( cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    proc.stdin.write("cd /tmp ;rts2-scriptexec -d {0} -s ' D {1} ' ; exit\n".format(ccdName, exp))
    return proc.stdout.readline()


if __name__ == '__main__':
    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf ccd exposure and RTS2 execution test')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--ccdname', dest='ccdName', action='store', default='andor3', help=': %(default)s, your ccd name, e.g. C0')

    args=parser.parse_args()

    print 'method1'
    try:
        fn= method1(debug=args.debug, ccdName=args.ccdName)
        if fn:
            print 'method2: Success!, {}'.format(fn)
        else:
            print 'method2: no file retrieved!'
    except Exception, e:
        print 'method1 error: {}'.format(e)

    print 'method2, the output you see is the stderr of rts2-scriptexec:'
    try:
        fn= method2(debug=args.debug, ccdName=args.ccdName)
        if fn:
            print 'method2: Success!, {}'.format(fn)
        else:
            print 'method2: no file retrieved!'
    except Exception, e:
        print 'method1 error: {}'.format(e)

    print
    print 'method3'
    try:
        srcFn=method3(debug=args.debug, ccdName=args.ccdName)
        print 'CCD: {}, file: {}'.format(args.ccdName, srcFn)
        print 'method3: Success!'
    except Exception, e:
        print 'method1 error: {}'.format(e)

    COUNTER=0
    parts=srcFn.split('.fits')
    newStoreFn= '{0}-{1:03d}.fits'.format(parts[0], COUNTER)
    print 'moving from /tmp/{0} to /tmp/{1}'.format(srcFn[:-2], newStoreFn)
    try:
        shutil.move(src='/tmp/{0}'.format(srcFn), dst='/tmp/{0}'.format(newStoreFn))
    except Exception, e:
        print 'could not move file: {}, error:{}'.format(srcFn, e)


    print 'DONE'
