#!/usr/bin/env python
#
# ISO 8601 manipulation routines.
# (C) 2012 Petr Kubanek, Institute of Physics
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

import calendar
import time
import re

def str(t):
	return time.strftime("%Y-%m-%dT%H:%M:%S",time.gmtime(t))

def ctime(s):
	# special format string - if only time is specified, it means tonight..
	match = re.match('^(\d+):(\d+):(\d+)$',s)
	if match:
		# check if time is after or before midday - before midday will be put to tomorrow, after midday will be left current
		lh = int(match.group(1)) - time.timezone / 3600
		if lh < 0:
			lh = lh + 24

		ld = None
		if lh > 12:
			ld = time.strftime('%Y%m%d',time.localtime(time.time())) 
		else:
			# for morning hours, add 1 day - put them to tomorrow
			ld = time.strftime('%Y%m%d',time.localtime(time.time() + 86400))
		s = ld + 'T' + s

	return calendar.timegm(time.strptime(s.replace("-", ""),"%Y%m%dT%H:%M:%S"))
