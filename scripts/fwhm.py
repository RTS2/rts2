#!/usr/bin/python

import rts2.sextractor
import sys
import pyfits
import math
import re
import atv
import rts2

from optparse import OptionParser


#FOC_POS = 'FOC_POS'
#non-RTS2 files usually have TELFOCUS
FOC_POS = 'TELFOCUS'

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

def processImage(fn,d,threshold=2.7,pr=False,ds9cat=None,bysegments=False,stars=[],printpeak=False):
	"""Process image, print its FWHM. Works with multi extension images.
	"""
	ff = pyfits.open(fn)

	if d:
		d.set('file ' + fn)
		#d.set('file mosaicimage iraf ' + fn)
	
	sc = rts2.Rts2Comm()

	sexcols = ['X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE','EXT_NUMBER','FLUX_BEST','BACKGROUND','XPEAK_IMAGE','YPEAK_IMAGE']

	c = rts2.sextractor.Sextractor(sexcols,threshold=threshold)
	c.runSExtractor(fn)
	c.sortObjects(2)

	atm = None

	try:
		atm = atv.ChannelsDTV(ff)
	except Exception,ex:
#		traceback.print_exc()
		pass

	for st in stars:
		# append distance - none and star number - to star list
		st.append(None)
		st.append(None)

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
		if atm:
			try:
				(dx,dy) = atm.xy2Detector('IM{0}'.format(segnum),x[0],x[1])
			except KeyError,ke:
				(dx,dy) = x[0:2]
		else:
			(dx,dy) = x[0:2]

		for st in stars:
			if st[0] == segnum:
				dist = math.sqrt((x[0]-st[1])**2+(x[1]-st[2])**2)
				if st[4] is None or st[4] > dist:
					st[4] = dist
					st[5] = x

		if x[3] == 0 and x[4] != 0:
			if d:
				print segnum,dx,dy
				d.set('regions','detector; circle {0} {1} 5 # color=red'.format(dx,dy))
				#d.set('regions','tile {0}\nimage; circle {1} {2} 10 # color=green'.format(segnum,x[0],x[1]))
			
			seg_fwhms[0].addFWHM(x[5],x[6],x[7])
			seg_fwhms[segnum].addFWHM(x[5],x[6],x[7])
		elif d:
			d.set('regions','# point {1} {2} # point = x 5 color=red'.format(segnum,dx,dy))
			#d.set('regions','# tile {0}\nphysical; point {1} {2} # point = x 5 color=red'.format(segnum,x[0],x[1]))

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
				sc.doubleValue('fwhm_foc{0}'.format(suffix),"focuser positon",ff[0].header[FOC_POS])
			except KeyError,er:
				pass
			try:
				zd = 90.0 - ff[0].header['TEL_ALT']
				if 90 > zd > 0:
					fz = seg_fwhms[x].fwhm * (math.cos(math.radians(zd)) ** 0.6)
					sc.doubleValue('fwhm_zd{0}'.format(suffix),"[deg] zenith distance of the FWHM measurement",zd)
					sc.doubleValue('fwhm_zenith{0}'.format(suffix),"estimated zenith FWHM",fz)
			except KeyError,er:
				pass
			try:
				sc.doubleValue('fwhm_az{0}'.format(suffix),"[deg] azimuth of the FWHM measurement",ff[0].header['TEL_AZ'])
			except KeyError,er:
				pass
			try:	
				sc.doubleValue('fwhm_airmass{0}'.format(suffix),"airmass of the FWHM measurement",ff[0].header['AIRMASS'])
			except KeyError,er:
				pass
		else:
			suffix = defsuffix + '_' + str(x)

		sc.doubleValue('fwhm{0}'.format(suffix),"calculated FWHM",seg_fwhms[x].fwhm)

		sc.doubleValue('fwhm_nstars{0}'.format(suffix),"number of stars for FWHM calculation",seg_fwhms[x].i)

	# print stars
	for st in stars:
		suf = st[3]
		sc.doubleValue('star_x_{0}'.format(suf),"x of star {0}".format(suf),st[5][0])
		sc.doubleValue('star_y_{0}'.format(suf),"y of star {0}".format(suf),st[5][1])
		sc.doubleValue('star_d_{0}'.format(suf),"distance of star {0}".format(suf),st[4])
		sc.doubleValue('flux_{0}'.format(suf),"flux of star {0}".format(suf),st[5][9])
		sc.statAdd('flux_stat_{0}'.format(suf),'flux statistics for {0}'.format(suf),5,st[5][9])
		sc.doubleValue('background_{0}'.format(suf),"flux of star {0}".format(suf),st[5][10])
		# peak coordinates
		if printpeak:
			x = st[5][11]
			y = st[5][12]
			sc.integerValue('xpeak_{0}'.format(suf),"X peak coordinate of {0}".format(suf),int(x))
			sc.integerValue('ypeak_{0}'.format(suf),"Y peak coordinate of {0}".format(suf),int(y))
			dat = int(ff[int(st[5][8])].data[y][x])
			sc.integerValue('peak_{0}'.format(suf),"peak value of star {0}".format(suf),dat)
			sc.statAdd('peak_stat_{0}'.format(suf),"peak value statistics".format(suf),5,dat)

	if d:
		d.set('regions','image; text 100 100 # color=red text={' + ('FWHM {0} foc {1} stars {2}').format(seg_fwhms[0].fwhm,ff[0].header[FOC_POS],seg_fwhms[0].i) + '}')

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('-d',help='display image and detected stars in DS9',action='store_true',dest='show_ds9',default=False)
	parser.add_option('--threshold',help='threshold for start detection',action='store',dest='threshold',default=3.0)
	parser.add_option('--print',help='print sextractor results',action='store_true',dest='pr',default=False)
	parser.add_option('--ds9cat',help='write DS9 catalogue file',action='store',dest='ds9cat')
	parser.add_option('--by-segments',help='calculate also FHWM values on segments',action='store_true',dest='bysegments')
	parser.add_option('--star-flux',help='calculate star FLUX at given position (seg:x:y:name)',action='append',dest='star_flux')
	parser.add_option('--peak',help='report peak XY and value',action='store_true',dest='peak',default=False)

	(options,args)=parser.parse_args()

	d = None
	if options.show_ds9:
		import ds9
		d = ds9.ds9('fwhm')
	stars=[]

	ma = re.compile('(\d+):(\d+):(\d+):(\S+)')

	if options.star_flux:
		for sf in options.star_flux:
			gr = ma.match(sf)
			if gr:
				stars.append([int(gr.group(1)),int(gr.group(2)),int(gr.group(3)),gr.group(4)])
			else:
				print >> sys.stderr, "Cannot parse star flux argument", sf
				sys.exit(1)

	for fn in args:
		processImage(fn,d,threshold=options.threshold,pr=options.pr,ds9cat=options.ds9cat,bysegments=options.bysegments,stars=stars,printpeak=options.peak)
