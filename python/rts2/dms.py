#!/usr/bin/env python
#
# DMS parse.
# (C) 2011 Petr Kubanek, Insiitute of Physics <kubanek@fzu.cz>
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

__author__ = 'kubanek@fzu.cz'

def parse(strin):
	"""Parse string in degrees notation to float"""
	mul=1.
	fraction=1.
	subres=0.

	ret=0.

	neg = False

	for x in strin:
		if x == ':':
			ret+=subres/mul
			subres=0
			fraction=1.
			mul *= 60.0
		elif x >= '0' and x <= '9':
			if fraction >= 1:
				subres = fraction*subres + int(x)
	  			fraction*=10
			else:
				subres = subres + int(x)*fraction
				fraction/=10
		elif x == '+' and mul == 1 and fraction == 1:
			neg = False
		elif x == '-' and mul == 1 and fraction == 1:
			neg = True
		elif x == '.':
			ret+=subres/mul
			fraction=0.1
			subres=0
		elif x == ' ':
			pass
		else:
			raise Exception('Cannot parse {0} - problem with {1}'.format(strin,x))  

	ret+=subres/mul
	if neg:
		ret*=-1
	return ret	

if __name__ == '__main__':
	print(parse('14:15:16.545')) 
	print(parse('-14:15:16'))
	print(parse('+14:15:16'))
	print(parse('+1-4:15:16')) 
