#!/usr/bin/python

# Basic libnova calculatons
# (C) 2011, Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA 02111-1307, USA.
#
# Or visit http://www.gnu.org/licenses/gpl.html.

from math import cos,sin,atan2,sqrt,radians,degrees

def angularSeparation(ra1,dec1,ra2,dec2):
	a1 = radians(ra1)
	d1 = radians(dec1)
	a2 = radians(ra2)
	d2 = radians(dec2)

	x = (cos(d1) * sin(d2)) - (sin(d1) * cos(d2) * cos(a2-a1))
	y = cos(d2) * sin(a2-a1)
	z = (sin (d1) * sin (d2)) + (cos(d1) * cos(d2) * cos(a2-a1))

	x = x * x;
	y = y * y;
	d = atan2(sqrt(x+y),z)

	return degrees(d)

if __name__ == '__main__':
	print angularSeparation(0,0,1,1)
	print angularSeparation(0,0,0,90)
	print angularSeparation(0,0,180,0)
	print angularSeparation(0,0,180,-40)
