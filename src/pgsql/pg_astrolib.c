/*!
 * @file Defines extension to PostgresSQL to enable libnova calculations
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

PG_FUNCTION_INFO_V1 (ln_angular_separation);
PG_FUNCTION_INFO_V1 (ln_airmass);

Datum
ln_angular_separation (PG_FUNCTION_ARGS)
{
  struct ln_equ_posn pos1, pos2;

  if (PG_ARGISNULL (0) || PG_ARGISNULL (1)
      || PG_ARGISNULL (2) || PG_ARGISNULL (3))
    PG_RETURN_NULL ();

  pos1.ra = PG_GETARG_FLOAT8 (0);
  pos1.dec = PG_GETARG_FLOAT8 (1);
  pos2.ra = PG_GETARG_FLOAT8 (2);
  pos2.dec = PG_GETARG_FLOAT8 (3);

  PG_RETURN_FLOAT8 (ln_get_angular_separation (&pos1, &pos2));
}

Datum
ln_airmass (PG_FUNCTION_ARGS)
{
  struct ln_equ_posn pos;
  struct ln_lnlat_posn obs;
  struct ln_hrz_posn hrz;
  // ra, dec, lng, lat, JD
  if (PG_ARGISNULL (0) || PG_ARGISNULL (1)
      || PG_ARGISNULL (2) || PG_ARGISNULL (3) || PG_ARGISNULL (4))
    PG_RETURN_NULL ();

  pos.ra = PG_GETARG_FLOAT8 (0);
  pos.dec = PG_GETARG_FLOAT8 (1);
  obs.lng = PG_GETARG_FLOAT8 (2);
  obs.lat = PG_GETARG_FLOAT8 (3);

  ln_get_hrz_from_equ (&pos, &obs, PG_GETARG_FLOAT8 (4), &hrz);

  PG_RETURN_FLOAT4 (ln_get_airmass (hrz.alt, 750));
}
