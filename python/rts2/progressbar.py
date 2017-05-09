# Prints to console pretty progress bar
#
# (C) 2016 Petr Kubanek <petr@kubanek.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import sys
import fcntl
import termios
import struct
import time
import atexit

def show_cursor():
	sys.stderr.write('\x1b[?;25;h')
	sys.stderr.flush()

atexit.register(show_cursor)

try:
	COLS = struct.unpack('hh',  fcntl.ioctl(sys.stderr, termios.TIOCGWINSZ, '1234'))[1]
except IOError as io:
	# outputing to file..
	COLS = 0

def progress(current, total):
	if COLS <= 0:
		return
	prefix = '{0:5} / {1}'.format(current, total)
	bar_start = ' ['
	bar_end = ']'

	bar_size = COLS - len(prefix + bar_start + bar_end)
	if current >= total:
		amount = bar_size
		remain = 0
	else:
		amount = int(current / (total / float(bar_size)))
		remain = bar_size - amount

	bar = '\x1b[7m' + ' ' * amount + '\x1b[0m' + ' ' * remain
	sys.stderr.write('\x1b[?;25;l' + prefix + bar_start + bar + bar_end + '\r')
	if current >= total:
		sys.stderr.write('\n\x1b[?;25;h')
	sys.stderr.flush()
