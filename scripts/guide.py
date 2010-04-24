#!/usr/bin/python

# Guiding script
# (C) 2010 Martin Jelinek & Petr Kubanek

import sys
import subprocess
import os
import rts2comm

class GuideScript (rts2comm.Rts2Comm):
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self):
		self.detect4g = "/home/mates/detect4g"
		self.big_x = 324
		self.big_y = 69
		self.big_w = 1210
		self.big_h = 955

		# 1/4 pixelu NF
		self.x_sensitivity = 0.15
		self.y_sensitivity = 0.15
	
		# how much of the detected offset to apply (to dump resonance)
		self.ra_aggresivity = 0.7
		self.dec_aggresivity = 0.7

	def runProgrammeGetArray(self,command):
		sb=subprocess.Popen(command,stdout=subprocess.PIPE)
		sb.wait()
		return sb.stdout.readline().split();

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
			values = self.runProgrammeGetArray([self.detect4g,image])
			x = float(values[0])
			y = float(values[1])

			if (abs(x - 15) < self.x_sensitivity and abs (y - 15) < self.y_sensitivity):
				self.log('I','autoguiding below sensitivity %f %f' % (x,y))
				self.delete(image)
				continue

			change = self.runProgrammeGetArray(['rts2-image', '-n', '-d %f:%f-15:15' % (x,y), image])
			ch_ra = float(change[0]) * self.ra_aggresivity
			ch_dec = float(change[1]) * self.dec_aggresivity

			self.log('I','autoguiding values loop %f %f change %f %f (%.1f %.1f)' % (x,y,ch_ra,ch_dec,ch_ra*3600,ch_dec*3600))
			self.incrementValue('OFFS','%f %f' % (ch_ra, ch_dec), 'T0')
			self.log('I','autoguiding move finished')
			# os.system ('cat %s | su petr -c "xpaset ds9 fits"' % (image))
			self.delete(image)

	def run(self):
		# First we should do target centering
		## but at this moment let's omit that phase
		# And when it is centered, do the guiding itself		

		# 1536x1024 => 324 69 1210 955 (50% of the chip)
		self.setValue('WINDOW','%d %d %d %d' % (self.big_x, self.big_y, self.big_w, self.big_h))
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
				
		self.setValue('exposure',2) # how long exposure...? We could auto-adjust this...

		self.setValue('OFFS','0 0', 'T0')

		image = self.exposure()

		values = self.runProgrammeGetArray([self.detect4g,image])
		x = int(float(values[0])) + self.big_x
		y = int(float(values[1])) + self.big_y
		self.log('I','values for autoguiding %d %d' % (x, y))
				
		# while we are current target
		self.doGuiding(x,y)

a = GuideScript()
a.run()
