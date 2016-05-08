/* 
 * Simplified perturbations model
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_SGP4__
#define __RTS2_SGP4__

#include <libnova/libnova.h>

namespace rts2sgp4
{

typedef struct elsetrec
{
  long int  satnum;
  int       epochyr, epochtynumrev;
  int       error;
  char      operationmode;
  char      init, method;

  /* Near Earth */
  int    isimp;
  double aycof  , con41  , cc1    , cc4      , cc5    , d2      , d3   , d4    ,
         delmo  , eta    , argpdot, omgcof   , sinmao , t       , t2cof, t3cof ,
         t4cof  , t5cof  , x1mth2 , x7thm1   , mdot   , nodedot, xlcof , xmcof ,
         nodecf;

  /* Deep Space */
  int    irez;
  double d2201  , d2211  , d3210  , d3222    , d4410  , d4422   , d5220 , d5232 ,
         d5421  , d5433  , dedt   , del1     , del2   , del3    , didt  , dmdt  ,
         dnodt  , domdt  , e3     , ee2      , peo    , pgho    , pho   , pinco ,
         plo    , se2    , se3    , sgh2     , sgh3   , sgh4    , sh2   , sh3   ,
         si2    , si3    , sl2    , sl3      , sl4    , gsto    , xfact , xgh2  ,
         xgh3   , xgh4   , xh2    , xh3      , xi2    , xi3     , xl2   , xl3   ,
         xl4    , xlamo  , zmol   , zmos     , atime  , xli     , xni;

  double a      , altp   , alta   , epochdays, jdsatepoch       , nddot , ndot  ,
         bstar  , rcse   , inclo  , nodeo    , ecco             , argpo , mo    ,
         no;
} elsetrec;

/**
 * Initialize SGP4 parameters from TLE values.
 */
int init (const char *tle1, const char *tle2, elsetrec *satrec);

/**
 * Propagates satellite position to given position. Set ro and vo vectors to
 * values calculated for given JD.
 */
int propagate (elsetrec *satrec, double JD, struct ln_rect_posn *ro, struct ln_rect_posn *vo);

void ln_lat_alt_to_parallax (struct ln_lnlat_posn *observer, double altitude, double *rho_cos, double *rho_sin);

void ln_observer_cartesian_coords (struct ln_lnlat_posn *observer, double rho_cos, double rho_sin, double JD, struct ln_rect_posn *observer_loc);

void ln_get_satellite_ra_dec_delta (struct ln_rect_posn *observer_loc, struct ln_rect_posn *satellite_loc, struct ln_equ_posn *equ, double *delta);

}

#endif // !__RTS2_SGP4__
