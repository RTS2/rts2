#!/usr/bin/env python
#
# (C) 2012 Petr Kubanek, Institute of Physics
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

from __future__ import absolute_import

import base64
import json as sysjson
import httplib
import urllib
import string

class Rts2JSON:
	def __init__(self,username=None,password=None,verbose=False):
		self.headers = {'Authorization':'Basic' + string.strip(base64.encodestring('{0}:{1}'.format(username,password)))}
		self.verbose = verbose
		self.hlib = self.newConnection()

	def newConnection(self):
		return httplib.HTTPConnection('localhost',8889)

	def loadJson(self,req,args=None):
		url = req
		if args:
			url += '?' + urllib.urlencode(args)
		self.hlib.request('GET', url, None, self.headers)
		r = None
		try:
			r = self.hlib.getresponse()
		except httplib.BadStatusLine,ex:
			# try to reload on broken connection
			self.hlib = self.newConnection()
			self.hlib.request('GET', url, None, self.headers)
			r = self.hlib.getresponse()
		d = r.read()
		if self.verbose:
			print url
			print d
		return sysjson.loads(d)
