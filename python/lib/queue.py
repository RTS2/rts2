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
import target
import xml.dom.minidom

QUEUE_FIFO                   = 0
QUEUE_CIRCULAR               = 1
QUEUE_HIGHEST                = 2
QUEUE_WESTEAST               = 3
QUEUE_WESTEAST_MERIDIAN      = 4
QUEUE_SET_TIMES              = 5

def _nanNone(num):
	return 'nan' if (num is None) else num 

def _xmlQueueBoolAttribute(value):
	return 'true' if value else 'false'

def _getXmlBoolAttribute(node,name):
	return node.getAttribute(name) == 'true'

class QueueEntry:
	"""Single queue entry. Provides methods to work on entry (observation request) level."""
	def __init__(self,id=None,start=None,end=None,qid=None):
		self.id = id
		self.start = start
		self.end = end
		self.qid = qid
		self.target = None
	
	def from_xml(self,el):
		self.id = el.getAttribute('id')
		start = el.getAttribute('start')
		if len(start)>0:
			self.start = iso8601.ctime(start)
		end = el.getAttribute('end')
		if len(end)>0:
			self.end = iso8601.ctime(end)
		self.qid = None

	def get_target(self):
		"""Return target object associated with the queue."""
		if self.target is None:
			self.target = target.Target(self.id)
			self.target.reload()
		return self.target
	
	def get_xmlnode(self,document):
		en = document.createElement('queueEntry')
		en.setAttribute('id',str(self.id))

		en.setAttribute('name',self.get_target().name)
		if self.start is not None:
			en.setAttribute('start',iso8601str(self.start))
		if self.end is not None:
			en.setAttribute('end',iso8601str(self.end))
		return en

class Queue:
	"""Queue abstraction. Provides methods for operation on the queue."""
	def __init__(self,name,service=None,queueType=QueueEntry):
		"""Create new queue. Service parameters specify RTS2 service name. Optional queueType allow
		user to overwrite QueueEntry elements created for queue entries."""
		self.name = name
		self.service = service
		self.entries = []
		self.queueing = None
		self.skip_below = True
		self.test_constr = True
		self.remove_executed = True
		self.queueType = queueType

		if service is None:
			self.service = json.getProxy().getDevicesByType(json.DEVICE_TYPE_SELECTOR)[0]

	def clear(self):
		"""Clear the queue."""
		json.getProxy().executeCommand(self.service,'clear {0}'.format(self.name))

	def load(self):
		"""Refresh queue from server."""
		json.getProxy().refresh(self.service)
		self.queueing = json.getProxy().getValue(self.service,self.name + '_queing')
		self.skip_below = json.getProxy().getValue(self.service,self.name + '_skip_below')
		self.test_constr = json.getProxy().getValue(self.service,self.name + '_test_constr')
		self.remove_executed = json.getProxy().getValue(self.service,self.name + '_remove_executed')

		ids = json.getProxy().getValue(self.service,self.name + '_ids')
		start = json.getProxy().getValue(self.service,self.name + '_start')
		end = json.getProxy().getValue(self.service,self.name + '_end')
		qid = json.getProxy().getValue(self.service,self.name + '_qid')

		self.entries = []

		for i in range(0,len(ids)):
			self.entries.append(self.queueType(ids[i],start[i],end[i],qid[i]))

	def save(self):
		"""Save queue settings to the server."""
		self.clear()
		json.getProxy().setValues({
			'{0}_queing'.format(self.name):self.queueing,
			'{0}_skip_below'.format(self.name):self.skip_below,
			'{0}_test_constr'.format(self.name):self.test_constr,
			'{0}_remove_executed'.format(self.name):self.remove_executed,
			'{0}_queing'.format(self.name):self.queueing
		},device=self.service)
		queue_cmd = ''
		for x in self.entries:
			queue_cmd += ' {0} {1} {2}'.format(x.id,_nanNone(x.start),_nanNone(x.end))
		json.getProxy().executeCommand(self.service,'queue_at {0}{1}'.format(self.name,queue_cmd))
	
	def add_target(self,id,start=None,end=None):
		"""Add target to queue."""
		json.getProxy().executeCommand(self.service,'queue_at {0} {1} {2} {3}'.format(self.name,id,_nanNone(start),_nanNone(end)))

	def get_XMLdoc(self):
		"""Serialize queue to XML document."""
		document = xml.dom.minidom.getDOMImplementation().createDocument('http://rts2.org','queue',None)
		self.to_xml(document,document.documentElement)
		return document

	def to_xml(self,document,node):
		node.setAttribute('skip_below',_xmlQueueBoolAttribute(self.skip_below))
		node.setAttribute('test_constr',_xmlQueueBoolAttribute(self.test_constr))
		node.setAttribute('remove_executed',_xmlQueueBoolAttribute(self.remove_executed))
		node.setAttribute('queueing',str(self.queueing))

		map(lambda x:node.appendChild(x.get_xmlnode(document)),self.entries)

	def from_xml(self,node):
		"""Construct queue from XML representation."""
		self.skip_below = _getXmlBoolAttribute(node,'skip_below')
		self.test_constr = _getXmlBoolAttribute(node,'test_constr')
		self.remove_executed = _getXmlBoolAttribute(node,'remove_executed')
		self.queueing = int(node.getAttribute('queueing'))

		for el in node.getElementsByTagName('queueEntry'):
			q = self.queueType()
			q.from_xml(el)
			self.entries.append(el)
	
	def load_xml(self,f):
		document = xml.dom.minidom.parse(f)
		self.from_xml(document.documentElement)

	def __str__(self):
		if len(self.entries):
			ids = map(lambda q:str(q.id), self.entries)
			return ' '.join(ids)
		return 'empty'	
