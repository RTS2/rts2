#!/usr/bin/python

# Guiding script
# (C) 2010 Martin Jelinek & Petr Kubanek

import sys
import subprocess

class Rts2Comm:
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self):
		self.detect4g = "/home/mates/detect4g"

	def getValue(self,value,device = None):
		"""Returns given value."""
		if (not (device is None)):
			print 'G',device,value
		else:
			print '?',value
		sys.stdout.flush()
		return sys.stdin.readline()

	def getValueFloat(self,value,device = None):
		"""Return value as float number."""
		return float(self.getValue(value,device))

	def getValueInteger(self,value,device = None):
		return int(self.getValue(value,device))

	def incrementValue(self,name,new_value,device = None):
		if (device is None):
			print "value",name,'+=',new_value
		else:
			print "V",device,name,'+=',new_value
		sys.stdout.flush()

	def setValue(self,name,new_value,device = None):
		if (device is None):
			print "value",name,'=',new_value
		else:
			print "V",device,name,'=',new_value
		sys.stdout.flush()
	
	def exposure(self, before_readout_callback = None):
		print "exposure"
		sys.stdout.flush()
		a = sys.stdin.readline()
		if (a != "exposure_end\n"):
			self.log('E', "invalid return from exposure - expected exposure_end, received " + a)
		if (not (before_readout_callback is None)):
			before_readout_callback()
		a = sys.stdin.readline()
		image,fn = a.split()
		return fn

	def __imageAction(self,action,imagename):
		print action,imagename
		sys.stdout.flush()
		return sys.stdin.readline()


	def rename(self,imagename,pattern):
		print "rename",imagename,pattern
		sys.stdout.flush()
		return sys.stdin.readline()
	
	def toFlat(self,imagename):
		return self.__imageAction("flat",imagename)

	def toDark(self,imagename):
		return self.__imageAction("dark",imagename)

	def toArchive(self,imagename):
		return self.__imageAction("archive",imagename)

	def toTrash(self,imagename):
		return self.__imageAction("trash",imagename)
	
	def delete(self,imagename):
		print "delete",imagename
		sys.stdout.flush()
	
	def log(self,level,text):
		print "log",level,text
		sys.stdout.flush()
	
	def doGuiding(self,x,y):
		"""Guide the star to X,Y."""
		self.setValue('SHUTTER','LIGHT')

		winfmt="%d %d %d %d" % (x-16,y-16,30,30)
		self.log('I','WINDOW ' + winfmt)
		self.setValue('WINDOW',winfmt)
			
		exp=2
		tar_SNR=10;  # target star errorbar in magnitude (for exposure optimization)

		current = self.getValueInteger('current','EXEC')

		while (True):
			next = self.getValueInteger('next','EXEC')

			# end if there is next target different from the current one
			if (next != current and next != -1):
				self.setValue('SHUTTER','LIGHT')
				return
				

			# how long exposure...? We could auto-adjust this...
			self.setValue('exposure',exp) 
			image = self.exposure()

			# now run sextractor to get the star center

			# i have this written in bash, so I'll call it, no haste in turning it into python, it's a sex wrapper anyway
			sb=subprocess.Popen([self.detect4g,image],stdout=subprocess.PIPE)
			sb.wait()
			values=sb.stdout.readline().split();
			x = float(values[0])
			y = float(values[1])

			self.log('I','values in autoguding loop %f %f' % (x,y))
				
			# & move the telescope 
				

#			self.delete(img)

	def run(self):
		# First we should do target centering
		## but at this moment let's omit that phase
		# And when it is centered, do the guiding itself		

		# 1536x1024 => 324 69 1210 955 (50% of the chip)
		self.setValue('WINDOW','324 69 1210 955')
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
				
		self.setValue('exposure',2) # how long exposure...? We could auto-adjust this...
		image = self.exposure()

		sb=subprocess.Popen([self.detect4g,image],stdout=subprocess.PIPE);
		sb.wait()
		values=sb.stdout.readline().split()
		x = int(float(values[0])) + 324
		y = int(float(values[1])) + 69
		self.log('I','values for autoguiding %d %d' % (x, y))
				
		# while we are current target
		self.doGuiding(x,y)

a = Rts2Comm()
a.run()
