/*
 * Defines extension to PostgresSQL to handle efectivelly WCS informations.
 * Copyright (C) 2002-2008 Petr Kubanek <petr@kubanek.net>
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

#define _GNU_SOURCE

#include <postgres.h>
#include <fmgr.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#include <ctype.h>
#include <stdio.h>

#include <math.h>

// so gcc will not complains..
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <rts2-config.h>

struct kwcs2
{
  int naxis1;			/* Number of pixels along x-axis */
  int naxis2;			/* Number of pixels along y-axis */
  // we assume type RA---TAN and DEC--TAN
  double crpix1;		/* Reference pixel coordinates */
  double crpix2;		/* Reference pixel coordinates */
  double crval1;		/* Coordinate at reference pixel in degrees */
  double crval2;		/* Coordinate at reference pixel in degrees */
  double cd1_1;			/* Rotation matrix */
  double cd1_2;
  double cd2_1;
  double cd2_2;
  double equinox;		/* Epoch of coordinates, for FK4/FK5 conversion */
};

// help macro definitions
#define DatumGetKwcs2P(X)	((struct kwcs2 *) DatumGetPointer(X))
#define Kwcs2PGetDatum(X)	PointerGetDatum(X)
#define PG_GETARG_KWCS2_P(n)	DatumGetKwcs2P(PG_GETARG_DATUM(n))
#define PG_RETURN_KWCS2_P(X)	return Kwcs2PGetDatum(X)

// registration of PG SQL functions
PG_FUNCTION_INFO_V1 (wcs2_in);
PG_FUNCTION_INFO_V1 (wcs2_out);
PG_FUNCTION_INFO_V1 (isinwcs2);
PG_FUNCTION_INFO_V1 (imgrange2);
PG_FUNCTION_INFO_V1 (img_wcs2_naxis1);
PG_FUNCTION_INFO_V1 (img_wcs2_naxis2);
PG_FUNCTION_INFO_V1 (img_wcs2_ctype1);
PG_FUNCTION_INFO_V1 (img_wcs2_ctype2);
PG_FUNCTION_INFO_V1 (img_wcs2_crpix1);
PG_FUNCTION_INFO_V1 (img_wcs2_crpix2);
PG_FUNCTION_INFO_V1 (img_wcs2_crval1);
PG_FUNCTION_INFO_V1 (img_wcs2_crval2);
PG_FUNCTION_INFO_V1 (img_wcs2_cd1_1);
PG_FUNCTION_INFO_V1 (img_wcs2_cd1_2);
PG_FUNCTION_INFO_V1 (img_wcs2_cd2_1);
PG_FUNCTION_INFO_V1 (img_wcs2_cd2_2);
PG_FUNCTION_INFO_V1 (img_wcs2_equinox);
// center RA and DEC
PG_FUNCTION_INFO_V1 (img_wcs2_center_ra);
PG_FUNCTION_INFO_V1 (img_wcs2_center_dec);

// helper
char *
get_next_token (char *start, char **next_start)
{
  while (*start)
    {
      if (isalnum (*start) || ispunct (*start))
	{
	  char *ret = start;
	  while (isalnum (*++start) || ispunct (*start));
	  if (*start)
	    {
	      *start = 0;
	      *next_start = ++start;
	    }
	  else
	    *next_start = start;

	  return ret;
	}
      start++;
    }
  return NULL;
}

Datum
wcs2_in (PG_FUNCTION_ARGS)
{
  char *imhead;
  char *cmd;

  char *next_start;
  struct kwcs2 *res;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  imhead = PG_GETARG_CSTRING (0);

  res = (struct kwcs2 *) palloc (sizeof (struct kwcs2));
  memset (res, 0, sizeof (struct kwcs2));

// parse input string
  while ((cmd = get_next_token (imhead, &next_start)) != NULL)
    {
      char *arg = get_next_token (next_start, &next_start);

      // conversion macros

#define ARG_CH(name,len)\
	      else if (strcasecmp (cmd, #name) == 0) \
		      strncpy (res->name, arg, len)

#define ARG_INT(name)\
	      else if (strcasecmp (cmd, #name) == 0)\
		      res->name = atoi (arg)

#define ARG_FLOAT(name)\
	      else if (strcasecmp (cmd, #name) == 0)\
		      res->name = atof (arg)

      if (arg == NULL)
	{
	  elog (NOTICE, "returning NULL");
	  PG_RETURN_NULL ();
	}
      ARG_INT (naxis1);
      ARG_INT (naxis2);
      ARG_FLOAT (crpix1);
      ARG_FLOAT (crpix2);
      ARG_FLOAT (crval1);
      ARG_FLOAT (crval2);
      ARG_FLOAT (cd1_1);
      ARG_FLOAT (cd1_2);
      ARG_FLOAT (cd2_1);
      ARG_FLOAT (cd2_2);
      ARG_INT (equinox);
      else
      {
	//char *warning = malloc (200 + strlen (cmd) + strlen (arg));
	//sprintf (warning, "unknow string %s %s", cmd, arg);
	//elog (NOTICE, warning);
	//free (warning);
	return -1;
      }
      imhead = next_start;
    }
  PG_RETURN_KWCS2_P (res);
}

Datum
wcs2_out (PG_FUNCTION_ARGS)
{
  char *out;
  struct kwcs2 *arg;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);

  out = palloc (500);
  snprintf (out, 500,
	    "NAXIS1 %i NAXIS2 %i CRPIX1 %.20f CRPIX2 %.20f CRVAL1 %.20f CRVAL2 %.20f CD1_1 %.20f CD1_2 %.20f CD2_1 %.20f CD2_2 %.20f EQUINOX %f",
	    arg->naxis1, arg->naxis2, arg->crpix1, arg->crpix2,
	    arg->crval1, arg->crval2, arg->cd1_1, arg->cd1_2,
	    arg->cd2_1, arg->cd2_2, arg->equinox);

  PG_RETURN_CSTRING (out);
}

double
deg2rad (double deg)
{
  return deg * M_PI / 180.0;
}

double
rad2deg (double rad)
{
  return rad * 180.0 / M_PI;
}

/*!
 * Returns true, if given ra and dec are on image.
 *
 * @pg_arg	ra [float4]
 * @pg_arg	dec [float4
 * @pg_arg	wcs [kwcs2]
 *
 * @pg_ret [boolean] true if is in, false if not, NULL on error
 */
Datum
isinwcs2 (PG_FUNCTION_ARGS)
{
  double ra;
  double dec;
  struct kwcs2 *arg;

  double phi, theta, r_theta;
  double crra, crdec;

  double px, py;
  double tx;

  if (PG_ARGISNULL (0) || PG_ARGISNULL (1) || PG_ARGISNULL (2))
    PG_RETURN_NULL ();

  ra = deg2rad (PG_GETARG_FLOAT8 (0));
  dec = deg2rad (PG_GETARG_FLOAT8 (1));
  arg = PG_GETARG_KWCS2_P (2);

  crra = deg2rad (arg->crval1);
  crdec = deg2rad (arg->crval2);

  // convert ra & dec to pixel coordinates
  // in the following text, we refer to equation obtained from
  // A&A 395, 1077-1122(2002) "Representations of celestial coordinates in FITS"
  // by M.R.Calabretta and E.W.Greisen
  // WARNING: arg (x,y) = atan2 (y,x); as arg is INVERSE tangent function (read top of page 1080) 
  // first calculate native coordinates

  phi = M_PI + atan2 (-cos (dec) * sin (ra - crra), sin (dec) * cos (crdec) - cos (dec) * sin (crdec) * cos (ra - crra));	// (5), p. 1080
  theta = asin (sin (dec) * sin (crdec) + cos (dec) * cos (crdec) * cos (ra - crra));	// (6), p. 1080

  if (theta < 0)
    PG_RETURN_BOOL (false);

  r_theta = cos (theta) / sin (theta);

  px = r_theta * sin (phi);
  py = -r_theta * cos (phi);

  // back to degrees
  px = rad2deg (px);
  py = rad2deg (py);

  // inverse of (1), p. 1078
  tx = px / (arg->cd1_1 + arg->cd1_2 * ((py - arg->cd2_1) / arg->cd2_2));
  py = (py - arg->cd2_1 * px) / arg->cd2_2;
  px = tx;

  // offset
  px += arg->crpix1;
  py += arg->crpix2;

  PG_RETURN_BOOL ((px >= 0 && py >= 0 && px < arg->naxis1
		   && py < arg->naxis2));
}

/*!
 * Calculate word coordinates of given pixel.
 */
void
RTS2pix2wcs (struct kwcs2 *arg, double x, double y, double *ra, double *dec)
{
  double ra0, dec0;
  double dx, dy, tx;
  double phi, theta, r_theta;

  ra0 = deg2rad (arg->crval1);
  dec0 = deg2rad (arg->crval2);

  // Offset from ref pixel
  dx = x - arg->crpix1;
  dy = y - arg->crpix2;

  // Scale and rotate using CD matrix
  tx = arg->cd1_1 * dx + arg->cd1_2 * dy;
  dy = arg->cd2_1 * dx + arg->cd2_2 * dy;
  dx = tx;

  dx = deg2rad (dx);
  dy = deg2rad (dy);

  phi = atan2 (dx, -dy);	// (14), p. 1085
  r_theta = sqrt (dx * dx + dy * dy);	// (15), p. 1085
  theta = atan2 (1, r_theta);	// (17), p. 1085

  *ra = ra0 + atan2 (-cos (theta) * sin (phi - M_PI), sin (theta) * cos (dec0) - cos (theta) * sin (dec0) * cos (phi - M_PI));	// (2), p. 1079
  *dec = asin (sin (theta) * sin (dec0) + cos (theta) * cos (dec0) * cos (phi - M_PI));	// (2), p. 1079

  *ra = rad2deg (*ra);
  *dec = rad2deg (*dec);
}

/*!
 * Returns string with image center coordinates.
 * 
 * Use wcsrange function.
 *
 * @pg_arg	wcs [kwcs2]
 *
 * @pg_ret [cstring] with image size.
 */
Datum
imgrange2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  char *buffer;
  double ra1, ra2, dec1, dec2;
  text *res;
  int l;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);

  buffer = (char *) palloc (200);

  RTS2pix2wcs (arg, 0, 0, &ra1, &dec1);
  RTS2pix2wcs (arg, arg->naxis1, arg->naxis2, &ra2, &dec2);

  l =
    snprintf (buffer, 200, "[%0.3f-%0.3f] [%0.3f-%0.3f]", ra1, dec1, ra2,
	      dec2);

  l++;

  res = (text *) palloc (VARHDRSZ + l);
#ifdef RTS2_HAVE_PGSQL_SET_VARSIZE
  SET_VARSIZE (res, VARHDRSZ + l);
#else
  VARATT_SIZEP (res) = VARHDRSZ + l;
#endif
  memcpy (VARDATA (res), buffer, l);

  pfree (buffer);
  PG_RETURN_TEXT_P (res);
}

/*!
 * Returns varius WCS stuff out of kwcs
 */
Datum
img_wcs2_naxis1 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_INT32 (arg->naxis1);
}

Datum
img_wcs2_naxis2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_INT32 (arg->naxis2);
}

Datum
img_wcs2_crpix1 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crpix1);
}

Datum
img_wcs2_crpix2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crpix2);
}

Datum
img_wcs2_crval1 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crval1);
}

Datum
img_wcs2_crval2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crval2);
}

Datum
img_wcs2_cd1_1 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->cd1_1);
}

Datum
img_wcs2_cd1_2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->cd1_2);
}

Datum
img_wcs2_cd2_1 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->cd2_1);
}

Datum
img_wcs2_cd2_2 (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->cd2_2);
}

Datum
img_wcs2_equinox (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->equinox);
}

Datum
img_wcs2_center_ra (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crval1);
}

Datum
img_wcs2_center_dec (PG_FUNCTION_ARGS)
{
  struct kwcs2 *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS2_P (0);
  PG_RETURN_FLOAT8 (arg->crval2);
}
