#!/usr/bin/python

# Photometry manipulation scripts
# Copyright (C) Petr Kubanek, Instritute of Physics <kubanek@fzu.cz>


import psycopg2

class Phot:
	def __init__(self):
		self.conn = psycopg2.connect('dbname=stars')
	
	def insert_object(self,ra,dec,gsc):
		cur = self.conn.cursor()
		cur.execute("SELECT nextval('objects_obj_id');")
		objid = cur.fetchone()[0]
		cur.execute('INSERT INTO objects (obj_id,obj_ra,obj_dec,obj_gsc) VALUES (%s,%s,%s,%s);',(objid,ra,dec,gsc,))
		self.conn.commit()
		cur.close()
		return objid
	
if __name__ == '__main__':
	import optparse

	parser = optparse.OptionParser()
	parser.add_option('--add-object',help='add object specified as ra:dec:gsc',action='store',dest='addobj')

	(options,args) = parser.parse_args()

	phot = Phot()

	if options.addobj:
		(ra,dec,gsc) = options.addobj.split(':')
		print phot.insert_object(ra,dec,gsc)
