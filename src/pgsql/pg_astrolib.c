/*!
 * @file Defines extension to PostgresSQL to handle efectivelly WCS informations.
 *
 * @author petr
 *
 * Copyright (C) 2002 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <libnova/libnova.h>

#include <math.h>
#include <postgres.h>
#include <fmgr.h>

#define AIRMASS_SCALE	750.0

PG_FUNCTION_INFO_V1 (obj_alt);
PG_FUNCTION_INFO_V1 (obj_az);
PG_FUNCTION_INFO_V1 (obj_rise);
PG_FUNCTION_INFO_V1 (obj_set);
PG_FUNCTION_INFO_V1 (obj_airmass);

Datum
obj_alt (PG_FUNCTION_ARGS)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  double ST;
  struct ln_hrz_posn hrz;
  int i;

  // get input parameters...
  for (i = 0; i < 5; i++)
    if PG_ARGISNULL
      (i) PG_RETURN_NULL ();

  object.ra = PG_GETARG_FLOAT8 (0);
  object.dec = PG_GETARG_FLOAT8 (1);
  ST = PG_GETARG_FLOAT8 (2);
  observer.lng = PG_GETARG_FLOAT8 (3);
  observer.lat = PG_GETARG_FLOAT8 (4);

  get_hrz_from_equ_sidereal_time (&object, &observer, ST, &hrz);

  PG_RETURN_FLOAT8 (hrz.alt);
}

Datum
obj_az (PG_FUNCTION_ARGS)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  double ST;
  struct ln_hrz_posn hrz;
  int i;

  // get input parameters...
  for (i = 0; i < 5; i++)
    if PG_ARGISNULL
      (i) PG_RETURN_NULL ();

  object.ra = PG_GETARG_FLOAT8 (0);
  object.dec = PG_GETARG_FLOAT8 (1);
  ST = PG_GETARG_FLOAT8 (2);
  observer.lng = PG_GETARG_FLOAT8 (3);
  observer.lat = PG_GETARG_FLOAT8 (4);

  get_hrz_from_equ_sidereal_time (&object, &observer, ST, &hrz);

  PG_RETURN_FLOAT8 (hrz.az);
}

Datum
obj_rise (PG_FUNCTION_ARGS)
{



}

Datum
obj_set (PG_FUNCTION_ARGS)
{


}

Datum
obj_airmass (PG_FUNCTION_ARGS)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  double ST;
  struct ln_hrz_posn hrz;
  double x;
  int i;

  // get input parameters...
  for (i = 0; i < 5; i++)
    if PG_ARGISNULL
      (i) PG_RETURN_NULL ();

  object.ra = PG_GETARG_FLOAT8 (0);
  object.dec = PG_GETARG_FLOAT8 (1);
  ST = PG_GETARG_FLOAT8 (2);
  observer.lng = PG_GETARG_FLOAT8 (3);
  observer.lat = PG_GETARG_FLOAT8 (4);

  get_hrz_from_equ_sidereal_time (&object, &observer, ST, &hrz);

  x = AIRMASS_SCALE * sin (M_PI * hrz.alt / 180.0);

  PG_RETURN_FLOAT8 (sqrt (x * x + 2 * AIRMASS_SCALE + 1) - x);
}
