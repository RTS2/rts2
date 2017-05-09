# Parse strings with suffixes for units
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

import argparse

def kmparse(string):
	"""Function to get float from sting with suffixes"""
	try:
		if string[-1].upper() == 'K':
			return float(string[:-1]) * 1000
		if string[-1].upper() == 'M':
			return float(string[:-1]) * 1000000
		else:
			return float(string)
	except Exception as ex:
		raise argparse.ArgumentTypeError('invalid suffix/number:{0}'.format(string))
