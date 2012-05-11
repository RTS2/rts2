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
import os
import threading

class Rts2JSON:
	def __init__(self,url='http://localhost:8889',username=None,password=None,verbose=False,http_proxy=None):
		use_proxy = False
		prefix = ''
		if os.environ.has_key('http_proxy') or http_proxy:
			if not(re.match('^http',url)):
				prefix = 'http://' + url
			if http_proxy:
				prefix = url
			url = os.environ['http_proxy']
			use_proxy = True

		url = re.sub('^http://','',url)
		slash = url.find('/')
		if slash >= 0:
			if not(use_proxy):
				prefix = url[slash:]
			self.host = url[:slash]
		else:
			self.host = url

		a = self.host.split(':')
		self.port = 80
		if len(a) > 2:
			raise Exception('too much : separating port and server')
		elif len(a) == 2:
			self.host = a[0]
			self.port = int(a[1])

		self.headers = {'Authorization':'Basic' + string.strip(base64.encodestring('{0}:{1}'.format(username,password)))}
		self.verbose = verbose
		self.hlib_lock = threading.Lock()
		self.hlib = self.newConnection()

	def newConnection(self):
		return httplib.HTTPConnection(self.host,self.port)

	def getResponse(self,path,args={},hlib=None):
		url = self.prefix + path + '?' + urllib.urlencode(args)
		if self.verbose:
			print _('retrieving {0}'.format(url))
		try:
			th = hlib
			if hlib is None:
				self.hlib_lock.acquire()
				th = self.hlib
			th.request('GET', url, None, self.headers)

			r = None
			try:
				r = th.getresponse()
			except httplib.BadStatusLine,ex:
				# try to reload
				th = self.newConnection()
				if hlib is None:
					self.hlib = th
				th.request('GET', url, None, self.headers)
				r = th.getresponse()
			except httplib.ResponseNotReady,ex:
				# try to reload
				th = self.newConnection()
				if hlib is None:
					self.hlib = th
				th.request('GET', url, None, self.headers)
				r = th.getresponse()

			if not(r.status == httplib.OK):
				s = 'execution of {0} failed with status {1} {2}'.format(url,r.status,r.reason)
				jr = json.load(r)
				raise Exception(jr['error'])
			return r
		except Exception,ec:
			print 'Cannot parse',url,':',ec
			raise ec
		finally:
			if hlib is None:
				self.hlib_lock.release()

	def loadData(self,path,args={},hlib=None):
		return self.getResponse(path,args,hlib).read()

	def loadJson(self,path,args={}):
		d = self.loadData(path,args)
		if self.verbose:
			print d
		return sysjson.loads()

class JSONProxy(Rts2JSON):
	"""Connection with managed cache of variables."""
	def __init__(self,username=None,password=None,verbose=False):
		Rts2JSON.__init__(self,username,password,verbose)
		self.devices = {}
