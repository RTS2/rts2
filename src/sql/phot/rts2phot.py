#!/usr/bin/python

# Photometry manipulation scripts
# Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>

import psycopg2
import math

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

	def add_measurements(self,objid,obs_id,img_id,ra,dec,airmass,x,x_err,y,y_err,ell,flux,flux_err,mag,mag_err):
		cur = self.conn.cursor()
		cur.execute('INSERT INTO measurements (obj_id,obs_id,img_id,meas_airmass,meas_x,meas_x_err,meas_y,meas_y_err,meas_ell,meas_flux,meas_flux_err,meas_mag,meas_mag_err) VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)',(objid,obs_id,img_id,ra,dec,airmass,x,x_err,y,y_err,ell,flux,flux_err,mag,mag_err))
		self.conn.commit()
		cur.close()
	
	def get_object(self,ra,dec,size=0.1):
		cur = self.conn.cursor()
		# select objects in square around star within given range
		cur.execute('SELECT obj_id,obj_ra,obj_dec,obj_gsc FROM objects WHERE abs(obj_ra - %s) < %s and abs (obj_dec - %s) < %s',(ra, size / math.cos (math.radians(dec))),dec,size)
		# order by distance 
		self.conn.commit()
		cur.close()

	def delete_measurements(self,obsid,imgid):
		cur = self.conn.cursor()
		cur.execute("DELETE FROM measurements WHERE obs_id = %s and img_id = %s",(obsid,imgid,))
		self.conn.commit()
		cur.close()
	
if __name__ == '__main__':
	import optparse
	import sys

	parser = optparse.OptionParser()
	parser.add_option('--add-object',help='add object specified as ra:dec:gsc',action='append',dest='addobj')
	parser.add_option('--add-measurements',help='add measurements to obs_id:img_id',action='store',dest='addmeas')
	parser.add_option('--delete-measurements',help='delete measurements belonging to obs_id:img_id',action='append',dest='delmeas')

	(options,args) = parser.parse_args()

	phot = Phot()

	if options.addobj:
		for x in options.addobj:
			(ra,dec,gsc) = x.split(':')
			print phot.insert_object(ra,dec,gsc)
	
	if options.addmeas:
		(obsid,imgid)=options.addmeas.split(':')
		line = sys.stdin.readline()
		while line:
			(ra,dec,airmass,x,x_err,y,y_err,ell,flux,flux_err,mag,mag_err) = line.split()
			objid = 1
			phot.add_measurements(objid,obsid,imgid,ra,dec,airmass,x,x_err,y,y_err,ell,flux,flux_err,mag,mag.err)

			line = sys.stdin.readline()

	if options.delmeas:
		for x in options.delmeas:
			(objid,imgid) = x.split(':')
			phot.delete_measurements(objid,imgid)
