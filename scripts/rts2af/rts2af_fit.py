#!/usr/bin/python
# (C) 2012, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts2af_fit.py 
#   
# This file is still work in progress
#
#   rts2af_fit.py is called by rts2af_analysis.py
#
#   It isn't intended to be used interactively. 
#
#   rts2af_fit.py's purpose is to fit FWHM and flux data
#   taken during an online (rts2af_acquire.py) or an offline
#   (rts2af_offline.py) run.
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

__author__ = 'markus.wildi@one-arcsec.org'


import sys
import matplotlib.pyplot as plt
import numpy as np
from scipy import optimize

# FWHM
fitfunc_fwhm = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)+ p[3] * (x ** 3)
errfunc_fwhm = lambda p, x, y, res, err: (y - fitfunc_fwhm(p, x)) / (res * err)
fitfunc_r_fwhm = lambda x, p0, p1, p2, p3: p0 + p1 * x + p2 * (x ** 2) + p3 * (x ** 3) 

# FLUX
fitfunc_flux = lambda p, x: p[0] * p[4] / ( p[4] + p[3] * (abs(x - p[1]) ** p[2]))
errfunc_flux = lambda p, x, y, res: (y - fitfunc_flux(p, x)) / res
# ToDo: is a bit dangerous
fitfunc_r_flux = lambda x, p0, p1, p2, p3, p4: 1./( p0 * p4 / ( p4 + p3 * (abs(x - p1) ** p2)))


if __name__ == '__main__':

    # legacy interface
    # 1  
    # UNK 
    # NoTemp 2012-06-30T00:09:35.794656 6 
    # /tmp/rts2af/rts2af-fit-autofocus-UNK-2012-07-06T04:19:35.633406-FWHM_IMAGE.dat 
    # /tmp/rts2af/rts2af-fit-autofocus-UNK-2012-07-06T04:19:35.633406-FLUX_MAX.dat /tmp/rts2af/rts2af-fit-2.png
    script= sys.argv[0].split('/')[-1]
    show_plot= int(sys.argv[1])
    filter_name= sys.argv[2]
    temperature=sys.argv[3]
    date= sys.argv[4]
    objects=sys.argv[5]
    file_fwhm= sys.argv[6]
    file_flux= sys.argv[7]
    file_plot=sys.argv[8]

    # FWHM
    pos_fwhm, fwhm, errx_fwhm, erry_fwhm = np.loadtxt(file_fwhm, unpack=True)

    par_fwhm= np.array([180., -9.0e-02, 1.0e-05, 0.])
    try:
        par_fwhm, flag_fwhm  = optimize.leastsq(errfunc_fwhm, par_fwhm, args=(pos_fwhm, fwhm, errx_fwhm, erry_fwhm))
    except:
        sys.exit(1)
    #print flag_fwhm
    min_focpos_fwhm = optimize.fmin(fitfunc_r_fwhm,(max(pos_fwhm)-min(pos_fwhm))/2.+ min(pos_fwhm),args=(par_fwhm), disp=0)
    val_fwhm= fitfunc_fwhm( par_fwhm, min_focpos_fwhm) 

    # flux
    pos_flux, flux, errx_flux, erry_flux = np.loadtxt(file_flux, unpack=True)
    par_flux=[100., min_focpos_fwhm, 2., 0.072, 1000. * val_fwhm]
    try:
        par_flux, flag_flux  = optimize.leastsq(errfunc_flux, par_flux, args=(pos_flux, flux, errx_flux))
    except:
        sys.exit(1)
    #print flag_flux

    max_focpos_flux = optimize.fmin(fitfunc_r_flux,10.,args=(par_flux), disp=0)
    val_flux= fitfunc_flux( par_flux, max_focpos_flux) 

    # print results
    print '{0}: date: {1}'.format(script, date)
    print '{0}: temperature: {1}'.format(script,temperature)
    print '{0}: objects: {1}'.format(script,objects)

    if( flag_fwhm==1):
        print '{0}: FWHM_FOCUS {1}, FWHM at Minimum {2}'.format(script, min_focpos_fwhm[0], val_fwhm[0])
    else:
        print 'FWHM_FOCUS {0}, {1}'.format('nan', 'nan')

    if( flag_flux==1):
        print '{0}: FLUX_FOCUS {1}, FLUX at Maximum {2}'.format(script, max_focpos_flux[0], val_flux[0])
    else:
        print '{0}: FLUX_FOCUS {1}, {2}'.format(script, 'nan', 'nan')

    if( flag_fwhm==1):
        print '{0}: FWHM parameters: {1}'.format(script, par_fwhm)
    if( flag_flux==1):
        print '{0}: flux parameters: {1}'.format(script, par_flux)
    
    x_fwhm = np.linspace(pos_fwhm.min(), pos_fwhm.max())
    x_flux = np.linspace(pos_flux.min(), pos_flux.max())

    plt.plot(pos_fwhm, fwhm, 'ro', color='blue')
    plt.errorbar(pos_fwhm, fwhm, xerr=errx_fwhm, yerr=erry_fwhm, ecolor='black', fmt=None)
    if( flag_fwhm==1):
        plt.plot(x_fwhm, fitfunc_fwhm(par_fwhm, x_fwhm), 'r-', color='blue')

    plt.plot(pos_flux, flux, 'ro', color='red')
    plt.errorbar(pos_flux, flux, xerr=errx_flux, yerr=erry_flux, ecolor='black', fmt=None)
    if( flag_flux==1):
        plt.plot(x_flux, fitfunc_flux(par_flux, x_flux), 'r-')


    plt.title('rts2af, {0},{1},{2},obj:{3},fw:{4:.0f},fl:{5:.0f}'.format(date, filter_name, temperature, objects, float(min_focpos_fwhm), float(max_focpos_flux)), fontsize=12)

    plt.xlabel('FOC_POS [tick]')
    plt.ylabel('FWHM (blue) [px], FLUX [a.u.]')
    plt.grid(True)
    plt.savefig(file_plot)
    if( show_plot==1):
        plt.show()
