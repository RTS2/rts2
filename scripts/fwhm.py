#!/usr/bin/python

import sextractor
import sys
import pyfits
import math

from optparse import OptionParser

FOC_POS = 'FOC_POS'
#non-RTS2 files usually have TELFOCUS
#FOC_POS = 'TELFOCUS'

class FWHM:
	"""Holds FWHM on segments."""
	def __init__(self):
		self.fwhm = 0
		self.a = 0
		self.b = 0
		self.i = 0
	
	def addFWHM(self,fwhm,a,b):
		self.fwhm += fwhm
		self.a += a
		self.b += b
		self.i += 1
	
	def average(self):
		if self.i == 0:
			return
		self.fwhm /= self.i
		self.a /= self.i
		self.b /= self.i	

def processImage(fn,d,threshold=2.7,pr=False,ds9cat=None,bysegments=False):
	"""Process image, print its FWHM. Works with multi extension images.
	"""
	ff = pyfits.fitsopen(fn)

	if d:
		d.set('file mosaicimage iraf ' + fn)

	sexcols = ['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE','EXT_NUMBER']

	c = sextractor.Sextractor(sexcols,threshold=threshold)
	c.runSExtractor(fn)
	c.sortObjects(2)

	# dump Sextractor to DS9 catalogue
	if ds9cat:
		cat = open(ds9cat,'w')
		cat.write('\t'.join(sexcols) + '\n')
		for x in c.objects:
			cat.write('\t'.join(map(lambda y:str(y),x)) + '\n')
		cat.close()

	seg_fwhms = map(lambda x:FWHM(),range(0,len(ff)+1))
	for x in c.objects:
		if pr:
			print '\t'.join(map(lambda y:str(y),x))
		segnum = int(x[8])
		if x[3] == 0 and x[4] != 0:
			if d:
				d.set('regions','tile {0}\nimage; circle {1} {2} 10 # color=green'.format(segnum,x[0],x[1]))
			
			seg_fwhms[0].addFWHM(x[5],x[6],x[7])
			seg_fwhms[segnum].addFWHM(x[5],x[6],x[7])
		elif d:
			d.set('regions','# tile {0}\nphysical; point {1} {2} # point = x 5 color=red'.format(segnum,x[0],x[1]))

	# average results
	map(FWHM.average,seg_fwhms)
  	# default suffix
	defsuffix = '_KCAM'
	try:
		defsuffix = '_' + ff[0].header['CCD_NAME']
	except KeyError,er:
		pass
	for x in range(0,len(seg_fwhms)):
	  	if x == 0:
			suffix = defsuffix
			try:
				print 'double fwhm_foc{0} "focuser positon" {1}'.format(suffix,ff[0].header[FOC_POS])
			except KeyError,er:
				pass
			try:
				zd = 90 - ff[0].header['TEL_ALT']
				fz = seg_fwhms[x].fwhm * (math.cos(math.radians(zd)) ** 0.6)
				print 'double fwhm_zd{0} "[deg] zenith distance of the FWHM measurement" {1}'.format(suffix,zd)
				print 'double fwhm_zenith{0} "estimated zenith FWHM" {1}'.format(suffix,fz)
			except KeyError,er:
				pass
			try:
				print 'double fwhm_az{0} "[deg] azimuth of the FWHM measurement" {1}'.format(suffix,ff[0].header['TEL_AZ'])
			except KeyError,er:
				pass
			try:	
				print 'double fwhm_airmass{0} "airmass of the FWHM measurement" {1}'.format(suffix,ff[0].header['AIRMASS'])
			except KeyError,er:
				pass
		else:
			suffix = defsuffix + '_' + str(x)

		print 'double fwhm{0} "calculated FWHM" {1}'.format(suffix,seg_fwhms[x].fwhm)

		print 'double fwhm_nstars{0} "number of stars for FWHM calculation" {1}'.format(suffix,seg_fwhms[x].i)

	if d:
		d.set('regions','image; text 100 100 # color=red text={' + ('FWHM {0} foc {1} stars {2}').format(seg_fwhms[0].fwhm,ff[0].header[FOC_POS],seg_fwhms[0].i) + '}')

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('-d',help='display image and detected stars in DS9',action='store_true',dest='show_ds9',default=False)
	parser.add_option('--threshold',help='threshold for start detection',action='store',dest='threshold',default=3.0)
	parser.add_option('--print',help='print sextractor results',action='store_true',dest='pr',default=False)
	parser.add_option('--ds9cat',help='write DS9 catalogue file',action='store',dest='ds9cat')
	parser.add_option('--by-segments',help='calculate also FHWM values on segments',action='store_true',dest='bysegments')

	(options,args)=parser.parse_args()

	d = None
	if options.show_ds9:
		import ds9
		d = ds9.ds9('fwhm')

	for fn in args:
		processImage(fn,d,threshold=options.threshold,pr=options.pr,ds9cat=options.ds9cat,bysegments=options.bysegments)
