#!/usr/bin/env python
#
# Queue JSON interface.
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

import json
import iso8601

QUEUE_FIFO                   = 0
QUEUE_CIRCULAR               = 1
QUEUE_HIGHEST                = 2
QUEUE_WESTEAST               = 3
QUEUE_WESTEAST_MERIDIAN      = 4
QUEUE_SET_TIMES              = 5

class QueueEntry:
	def __init__(self, id=None, start=None, end=None, qid=None):
		self.id = id
		self.start = start
		self.end = end
		self.qid = qid
	
	def from_xml(self,el):
		self.id = el.getAttribute('id')
		start = el.getAttribute('start')
		if len(start)>0:
			self.start = iso8601.ctime(start)
		end = el.getAttribute('end')
		if len(end)>0:
			self.end = iso8601.ctime(end)
		self.qid = None

class Queue:
	"""Queue abstraction. Provides methods for operation on the queue."""
	entries = []
	queing = None
	skip_below = False
	test_constr = False
	
	def __init__(self,name,js,service=None):
		self.name = name
		self.js = js
		self.service = service
		if service is None:
			self.service = js.getDevicesByType(json.DEVICE_TYPE_SELECTOR)[0]

	def load(self):
		"""Refresh queue from server"""
		self.js.refresh(self.service)
		self.queing = self.js.getValue(self.service,self.name + '_queing')
		self.skip_below = self.js.getValue(self.service,self.name + '_skip_below')
		self.test_constr = self.js.getValue(self.service,self.name + '_test_constr')

		ids = self.js.getValue(self.service,self.name + '_ids')
		start = self.js.getValue(self.service,self.name + '_start')
		end = self.js.getValue(self.service,self.name + '_end')
		qid = self.js.getValue(self.service,self.name + '_qid')

		self.entries = []

		for i in range(0,len(ids)):
			self.entries.append(ids[i],start[i],end[i],qid[i])

	def save(self):
		"""Save queue settings to the server."""
		self.js.setValues({
			'{1}_queing'.format(service,self.name):self.queing,
			'{1}_skip_below'.format(self.name):self.skip_below,
			'{1}_test_constr'.format(self.name):self.test_constr
		},device=self.service)
