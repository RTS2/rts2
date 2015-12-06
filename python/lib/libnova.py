#!/usr/bin/python
#
# Basic libnova calculatons
# (C) 2011, Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

from numpy import cos,sin,arctan2,sqrt,radians,degrees

def angular_separation(ra1,dec1,ra2,dec2):
	a1 = radians(ra1)
	d1 = radians(dec1)
	a2 = radians(ra2)
	d2 = radians(dec2)

	x = (cos(d1) * sin(d2)) - (sin(d1) * cos(d2) * cos(a2-a1))
	y = cos(d2) * sin(a2-a1)
	z = (sin (d1) * sin (d2)) + (cos(d1) * cos(d2) * cos(a2-a1))

	x = x * x;
	y = y * y;
	d = arctan2(sqrt(x+y),z)

	return degrees(d)

if __name__ == '__main__':
	print angular_separation(0,0,1,1)
	print angular_separation(0,0,0,90)
	print angular_separation(0,0,180,0)
	print angular_separation(0,0,180,-40)
