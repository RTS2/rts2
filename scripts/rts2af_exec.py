#!/usr/bin/python
# (C) 2011, Markus Wildi, markus.wildi@one-arcsec.org
#
#   helper script for rts2af_fwhm.py
#   Do not invoke it directly
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

import rts2comm
import logging
import time

logging.basicConfig(filename='/var/log/rts2-debug', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logging.info('exec.py: setting next target 5 via EXEC next')
r2c= rts2comm.Rts2Comm()
r2c.setValue('next', 5, 'EXEC')
# ToDo log later if it works
time.sleep(2)
id= r2c.getValueInteger('next','EXEC')
logging.info('exec.py: setting next target via EXEC retrieved id={0}, should be 5'.format(id))

# one normally does not see the second log message
# often (always?) rts2-scriptexec dies with
#*** glibc detected *** rts2-scriptexec: free(): invalid pointer: 0x00000000008661e0 ***
#======= Backtrace: =========
#/lib64/libc.so.6[0x7fbc1b77fc76]
#/lib64/libc.so.6(cfree+0x6c)[0x7fbc1b78496c]
#rts2-scriptexec[0x42c3e8]
#rts2-scriptexec[0x40f07c]
#rts2-scriptexec[0x40fc0f]
#/lib64/libc.so.6(__libc_start_main+0xfd)[0x7fbc1b72ba7d]
#rts2-scriptexec[0x40cb39]
#======= Memory map: ========
# ...
