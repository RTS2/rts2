#!/bin/bash
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
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

./rts2saf_analyze.py --base ./samples/ --toc  --fit --ds9
./rts2saf_analyze.py --base ./samples/ --toc  --flux --fit --ds9
./rts2saf_analyze.py --base ./samples/ --toc  --flux --fit --ds9 --assoc

./rts2saf_analyze.py --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.2 --assoc
./rts2saf_analyze.py --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.5 --assoc
./rts2saf_analyze.py --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.9 --assoc


./rts2saf_imgp.py ./imgp/20131011054939-621-RA.fits --toc

echo "Watch output in /var/log/rts2-debug MUST be writeable for current user"
./rts2saf_fwhm.py --config unittest/rts2saf-no-filter-wheel.cfg --fitsFn ./imgp/20131011054939-621-RA.fits
tail -200 /var/log/rts2-debug
echo "DONE"

