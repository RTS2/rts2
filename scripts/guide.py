#!/usr/bin/python

# Guiding script
# (C) 2010 Martin Jelinek & Petr Kubanek

import sys
import subprocess
import rts2comm

class GuideScript (rts2comm.Rts2Comm):
	"""Class for communicating with RTS2 in exe command."""
	def __init__(self):
		self.detect4g = "/home/mates/detect4g"
		self.big_x = 324
		self.big_y = 69
		self.big_w = 1210
		self.big_h = 955

	def runProgrammeGetArray(self,command):
		sb=subprocess.Popen([self.detect4g,image],stdout=subprocess.PIPE)
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

			change = self.runProgrammeGetArray(['rts2-image', '-n', '-d 15:15-%f:%f' % (x,y), image])
			ch_ra = float(change[0])
			ch_dec = float(change[1])

			self.log('I','values in autoguding loop %f %f change %f %f' % (x,y,ch_ra,ch_dec))
			self.setValue('OFFS','%f %f' % (ch_ra, ch_dec), 'T0')
			os.system ('cat %s | xpaset ds9 fits' % (image))
			self.delete(img)

	def run(self):
		# First we should do target centering
		## but at this moment let's omit that phase
		# And when it is centered, do the guiding itself		

		# 1536x1024 => 324 69 1210 955 (50% of the chip)
		self.setValue('WINDOW','%d %d %d %d' % (self.big_x, self.big_y, self.big_w, self.big_h))
		# make sure we are taking light images..
		self.setValue('SHUTTER','LIGHT')
				
		self.setValue('exposure',2) # how long exposure...? We could auto-adjust this...
		image = self.exposure()

		values = self.runProgrammeGetArray([self.detect4g,image])
		x = int(float(values[0])) + self.big_x
		y = int(float(values[1])) + self.big_y
		self.log('I','values for autoguiding %d %d' % (x, y))
				
		# while we are current target
		self.doGuiding(x,y)

a = GuideScript()
a.run()
