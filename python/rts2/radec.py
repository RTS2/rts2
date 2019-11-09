#!/usr/bin/env python
#
# RADEC parser.
# (C) 2013 Petr Kubanek <petr@kubanek.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

__author__ = 'petr@kubanek.net'

from . import dms

def parse(strin):
	"""Parse string in RADEC to two floats. RA is expected to be in hours(:minutes:seconds). Both RA and DEC are returned in degrees."""
	(ra,dec) = strin.split()
	return (dms.parse(ra) * 15.0, dms.parse(dec))

if __name__ == '__main__':
	print(parse('12:30:40 14:15:16.545')) 
	print(parse('12:30:40 -14:15:16'))
	print(parse('12:30:40 +14:15:16'))
	print(parse('+1-4:15:16')) 
