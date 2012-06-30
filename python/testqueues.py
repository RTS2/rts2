#!/usr/bin/python
import rts2.queue
import rts2.json
import sys

rts2.json.createProxy('http://localhost:8889',username='petr',password='test')

q=rts2.queue.Queue('manual')
print q.service
q.load()
for x in q.entries:
	print x.id,x.start,x.end,x.qid


q.addTarget(1000)

q.load()
for x in q.entries:
	print x.id,x.start,x.end,x.qid

q.save()

q.load()
for x in q.entries:
	print x.id,x.start,x.end,x.qid

doc=q.getXMLDoc()

f = open('/tmp/test.queu','w+')
doc.writexml(f,addindent='\t',newl='\n')
f.close()

f = open('/tmp/test.queu','r')
q.loadXml(f)
f.close()
