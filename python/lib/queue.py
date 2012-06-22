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

def _nanNone(num):
	return 'nan' if (num is None) else num 

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
	queueing = None
	skip_below = True
	test_constr = True
	remove_executed = True
	
	def __init__(self,name,js,service=None):
		self.name = name
		self.js = js
		self.service = service
		if service is None:
			self.service = js.getDevicesByType(json.DEVICE_TYPE_SELECTOR)[0]

	def load(self):
		"""Refresh queue from server"""
		self.js.refresh(self.service)
		self.queueing = self.js.getValue(self.service,self.name + '_queing')
		self.skip_below = self.js.getValue(self.service,self.name + '_skip_below')
		self.test_constr = self.js.getValue(self.service,self.name + '_test_constr')
		self.remove_executed = self.js.getValue(self.service,self.name + '_remove_executed')

		ids = self.js.getValue(self.service,self.name + '_ids')
		start = self.js.getValue(self.service,self.name + '_start')
		end = self.js.getValue(self.service,self.name + '_end')
		qid = self.js.getValue(self.service,self.name + '_qid')

		self.entries = []

		for i in range(0,len(ids)):
			self.entries.append(QueueEntry(ids[i],start[i],end[i],qid[i]))

	def save(self):
		"""Save queue settings to the server."""
		self.js.executeCommand(self.service,'clear {0}'.format(self.name))
		self.js.setValues({
			'{0}_queing'.format(self.name):self.queueing,
			'{0}_skip_below'.format(self.name):self.skip_below,
			'{0}_test_constr'.format(self.name):self.test_constr
		},device=self.service)
		queue_cmd = ''
		for x in self.entries:
			queue_cmd += ' {0} {1} {2}'.format(x.id,_nanNone(x.start),_nanNone(x.end))
		self.js.executeCommand(self.service,'queue_at {0}{1}'.format(self.name,queue_cmd))
	
	def addTarget(self,id,start=None,end=None):
		"""Add target to queue."""
		self.js.executeCommand(self.service,'queue_at {0} {1} {2} {3}'.format(self.name,id,_nanNone(start),_nanNone(end)))
