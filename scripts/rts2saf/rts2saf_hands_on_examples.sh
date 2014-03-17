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

./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples/ --toc  --fit --ds9
./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples/ --toc  --flux --fit --ds9
./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples/ --toc  --flux --fit --ds9 --assoc

./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.2 --assoc
./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.5 --assoc
./rts2saf_analyze.py --conf ./configs/hands_on/rts2saf.cfg  --base ./samples_bootes2/  --toc --flux --fit --ds9 --frac 0.9 --assoc


./rts2saf_imgp.py --conf ./configs/hands_on/rts2saf.cfg  ./imgp/20131011054939-621-RA.fits --toc

./rts2saf_fwhm.py --config ./configs/hands_on/rts2saf.cfg --fitsFn ./imgp/20131011054939-621-RA.fits
echo "DONE"

