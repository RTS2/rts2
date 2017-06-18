/* Header file dome target azimuth calculation.  */
/* Copyright (C) 2010, Markus Wildi */

/* The transformations are based on the paper Matrix Method for  */
/* Coodinates Transformation written by Toshimi Taki  */
/* (http://www.asahi-net.or.jp/~zs3t-tk).  */

/* Documentation: */
/* https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Robotic_ObsVermes#Telescope */

/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef  __RTS_DOME_TARGET_AZIMUTH__
#define __RTS_DOME_TARGET_AZIMUTH__

struct geometry {
  double  x_m;
  double  y_m;
  double  z_m;
  double  p;
  double  q;
  double  r;
  double r_D ;
}  ;
struct tk_geometry {
  double xd ;
  double zd ;
  double rdec ;
  double rdome ;
}  ;

#ifdef __cplusplus
extern "C"
{
#endif
double dome_target_az( struct ln_equ_posn tel_eq, struct ln_lnlat_posn obs_location, struct geometry obs) ;
#ifdef __cplusplus
}
#endif
#endif //  __RTS_DOME_TARGET_AZIMUTH__
