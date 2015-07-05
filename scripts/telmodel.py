#!/usr/bin/env python
from astropy.coordinates import SkyCoord,EarthLocation,AltAz
from astropy import units as u
from astropy.time import Time
import random

obs = EarthLocation(lat=20*u.deg,lon=-110*u.deg,height=3000*u.m)
obstime = Time('2015-07-04 22:33:00')

# get random alt/az
for az in range(0,360,20):
	t_alt = (45 + random.random() * 3) * u.deg
	t_az = (az + random.random() * 3) * u.deg

	hrz = SkyCoord(alt=t_alt,az=t_az,frame='altaz',location=obs,obstime=obstime)
	equ = hrz.icrs

	ra = equ.ra*u.deg
	dec = equ.dec*u.deg

	print ra.value,dec.value
