/* Header file pier telescope collision.  */
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

#ifndef __RTS_PIER_COLLISION__
#define __RTS_PIER_COLLISION__

#define NO_COLLISION             0
#define NO_DANGER                1
#define COLLIDING                2
#define WITHIN_DANGER_ZONE       3
#define WITHIN_DANGER_ZONE_ABOVE 4
#define WITHIN_DANGER_ZONE_BELOW 5

#ifdef __cplusplus
extern "C"
{
#endif

int pier_collision( struct ln_equ_posn *tel_equ, struct ln_lnlat_posn *obs) ;

#ifdef __cplusplus
}
#endif
#endif // __RTS_PIER_COLLISION__
