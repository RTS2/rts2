#!/usr/bin/python

# (C) 2014, Markus Wildi, markus.wildi@bluewin.ch
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


local_dir = './rts2saf'

def frostedDirectory():
    if not os.path.isdir(local_dir):
        print 'rts2saf_frosted.py: {} not found in local directory, change to ~/rts-2/scripts/rts2saf, exiting'.format(local_dir)
        sys.exit(1)


def executeFrosted():
    cmdL = ['frosted', '*py', 'rts2saf/*py', './unittest/*py']
    cmd = ' '.join(cmdL)
    stdo, stde = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()

    # ToDo
    if len(stdo) > 1471:
        print stdo

    if stde:
        print stde

if __name__ == '__main__':

    try:
        subprocess.Popen(['frosted', '/dev/null'])
    except Exception, e:
        print 'frosted not found, do: sudo pip install frosted, exiting'
        sys.exit(1)

    executeFrosted()
    print 'DONE'
