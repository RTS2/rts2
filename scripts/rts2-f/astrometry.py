#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
# (C) 2011, Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#   usage 
#   img_astrometry.py fits_filename
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'petr@fzu.cz'

import os
import shutil
import string
import subprocess
import sys
import re
import time
import pyfits
import tempfile
import pynova
import math

import dms

class imgAstrometryScript:
    """calibrate a fits image with astrometry.net."""
    def __init__(self,fits_file,odir=None,scale_relative_error=0.05,astrometry_bin='/home/petr/astrometry/bin'):
        self.scale_relative_error=scale_relative_error
	self.astrometry_bin=astrometry_bin

	self.odir = odir
	if self.odir is None:
		self.odir=tempfile.mkdtemp()

        self.infpath=self.odir + '/input.fits'
        shutil.copy(fits_file, self.infpath)

    def run(self,scale=None,ra=None,dec=None):

        solve_field=[self.astrometry_bin + '/solve-field', '-D', self.odir,'--no-plots', '--no-fits2fits']

	if scale is not None:
            scale_low=scale*(1-self.scale_relative_error)
            scale_high=scale*(1+self.scale_relative_error)
	    solve_field.append('-u')
	    solve_field.append('app')
	    solve_field.append('-L')
	    solve_field.append(str(scale_low))
	    solve_field.append('-H')
	    solve_field.append(str(scale_high))

	if ra is not None and dec is not None:
	    solve_field.append('--ra')
	    solve_field.append(ra)
	    solve_field.append('--dec')
	    solve_field.append(dec)
	    solve_field.append('--radius')
	    solve_field.append('5')

	solve_field.append(self.infpath)
	    
        proc=subprocess.Popen(solve_field,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

	radecline=re.compile('Field center: \(RA H:M:S, Dec D:M:S\) = \(([^,]*),(.*)\).')

	ret = None

	while True:
	    a=proc.stdout.readline()
	    if a == '':
	       break
	    match=radecline.match(a)
	    if match:
	       ret=[dms.parse(match.group(1)),dms.parse(match.group(2))]
	       
        # cleanup
        shutil.rmtree(self.odir)

	return ret

if __name__ == '__main__':
    if len(sys.argv) <= 2:
        print 'Usage: %s <odir> <fits filename> <xoffs> <yoffs> <origname> <obs_id> <imgid>' % (sys.argv[0])
        sys.exit(1)

    a = imgAstrometryScript(sys.argv[2],sys.argv[1])

    ra = dec = None

    ff=pyfits.fitsopen(sys.argv[2],'readonly')
    ra=ff[0].header['RA']
    dec=ff[0].header['DEC']
    object=ff[0].header['OBJECT']
    ff.close()

    xoffs = 0
    yoffs = 0

    if len(sys.argv) >= 5:
	xoffs = float(sys.argv[3])
	yoffs = float(sys.argv[4])

    num = -1
    try:
    	    fn = os.path.basename(sys.argv[5])
	    num = int(fn.split('.')[0])
    except Exception,ex:
    	    pass

    ret=a.run(scale=0.67,ra=ra,dec=dec)

    if ret:
	    raorig=dms.parse(ra)*15.0
	    decorig=dms.parse(dec)

	    raastr=float(ret[0])*15.0
	    decastr=float(ret[1])

	    err = pynova.angularSeparation(raorig,decorig,raastr+xoffs/3600,decastr+yoffs/3600.0)

	    ra_err = math.cos(math.radians(decorig))*((raorig-raastr)-xoffs/3600.0)
	    dec_err = decorig-decastr-yoffs/3600.0

	    print "corrwerr 1 {0:.10f} {1:.10f} {2:.10f} {3:.10f} {4:.10f}".format(raastr + xoffs/3600.0, decastr + yoffs/3600.0, ra_err, dec_err, err)

	    import rts2comm
	    c = rts2comm.Rts2Comm()
	    c.doubleValue('real_ra','[hours] image ra ac calculated from astrometry',raastr)
	    c.doubleValue('real_dec','[deg] image dec as calculated from astrometry',decastr)

	    c.doubleValue('tra','[hours] image ra ac calculated from astrometry',raorig)
	    c.doubleValue('tdec','[deg] image dec as calculated from astrometry',decorig)

	    c.doubleValue('xoffs','[arcsec] x offset',xoffs)
	    c.doubleValue('yoffs','[arcsec] y offset',yoffs)

	    c.doubleValue('ora','[arcsec] offsets ra ac calculated from astrometry',ra_err*3600.0)
	    c.doubleValue('odec','[arcsec] offsets dec as calculated from astrometry',dec_err*3600.0)

	    c.stringValue('object','astrometry object',object)
	    c.integerValue('img_num','last astrometry number',num)

	    os.system('/home/petr/rts2-sys/bin/rts2-image -i --camera KCAM --telescope FLWO48 --obsid {0} --imgid {1} --ra-err {2} --dec-err {3} --err {4} {5}'.format(sys.argv[6],sys.argv[7],ra_err,dec_err,err,sys.argv[5]))

