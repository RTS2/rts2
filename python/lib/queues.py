
# Queues JSON interface.
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

import queue
import json
import xml.dom.minidom

class Queues:
	def __init__(self):
		self.queues = {}
	
	def load(self):
		for d in json.getProxy().getDevicesByType(json.DEVICE_TYPE_SELECTOR):
			for q in json.getProxy().getValue(d, 'queue_names', refresh_not_found=True)[:-1]:
				if q not in self.queues.keys():
					self.queues[q] = queue.Queue(q, service=d)
				self.queues[q].load()
	
	def load_xml(self, fn, queues=None):
		f = open(fn, 'r')
		document = xml.dom.minidom.parse(f)
		f.close()
		self.queues = {}
		self.from_xml(document.documentElement, queues)

	def from_xml(self, node, queues=None):
		for qu in node.getElementsByTagName('queue'):
		  	qname = qu.getAttribute('name')
			if queues is not None and not(qname in queues):
				continue
			q = queue.Queue(qname)
			q.from_xml(qu)
			self.queues[qname] = q

	def save(self):
		map(lambda q: self.queues[q].save(), self.queues)

	def save_xml(self, fn):
		document = self.get_xml()
		f = open(fn, 'w')
		document.writexml(f, addindent='\t', newl='\n')
		f.close()

	def get_xml(self, queues=None):
		document = xml.dom.minidom.getDOMImplementation().createDocument('http://rts2.org', 'queues', None)
		for q in self.queues:
			if queues is not None and not(q in queues):
				continue
			node = document.createElement('queue')
			node.setAttribute('name', q)
			self.queues[q].to_xml(document, node)
			document.documentElement.appendChild(node)
		return document	
