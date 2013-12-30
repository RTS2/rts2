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

#include <libwcs/wcs.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>

// so gcc will not complains..
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <rts2-config.h>

struct kwcs
{
  int naxis1;			/* Number of pixels along x-axis */
  int naxis2;			/* Number of pixels along y-axis */
  char ctype1[15];		/* FITS WCS projection for axis 1 */
  char ctype2[15];		/* FITS WCS projection for axis 2 */
  double crpix1;		/* Reference pixel coordinates */
  double crpix2;		/* Reference pixel coordinates */
  double crval1;		/* Coordinate at reference pixel in degrees */
  double crval2;		/* Coordinate at reference pixel in degrees */
  // double  *cd;            /* Rotation matrix, used if not NULL */
  double cdelt1;		/* scale in degrees/pixel, if cd is NULL */
  double cdelt2;		/* scale in degrees/pixel, if cd is NULL */
  double crota;			/* Rotation angle in degrees, if cd is NULL */
  int equinox;			/* Equinox of coordinates, 1950 and 2000 supported */
  double epoch;			/* Epoch of coordinates, for FK4/FK5 conversion */
};

// help macro definitions
#define DatumGetKwcsP(X)	((struct kwcs *) DatumGetPointer(X))
#define KwcsPGetDatum(X)	PointerGetDatum(X)
#define PG_GETARG_KWCS_P(n)	DatumGetKwcsP(PG_GETARG_DATUM(n))
#define PG_RETURN_KWCS_P(X)	return KwcsPGetDatum(X)

// registration of PG SQL functions
PG_FUNCTION_INFO_V1 (wcs_in);
PG_FUNCTION_INFO_V1 (wcs_out);
PG_FUNCTION_INFO_V1 (isinwcs);
PG_FUNCTION_INFO_V1 (imgrange);
PG_FUNCTION_INFO_V1 (img_wcs_naxis1);
PG_FUNCTION_INFO_V1 (img_wcs_naxis2);
PG_FUNCTION_INFO_V1 (img_wcs_ctype1);
PG_FUNCTION_INFO_V1 (img_wcs_ctype2);
PG_FUNCTION_INFO_V1 (img_wcs_crpix1);
PG_FUNCTION_INFO_V1 (img_wcs_crpix2);
PG_FUNCTION_INFO_V1 (img_wcs_crval1);
PG_FUNCTION_INFO_V1 (img_wcs_crval2);
PG_FUNCTION_INFO_V1 (img_wcs_cdelt1);
PG_FUNCTION_INFO_V1 (img_wcs_cdelt2);
PG_FUNCTION_INFO_V1 (img_wcs_crota);
PG_FUNCTION_INFO_V1 (img_wcs_epoch);
// center RA and DEC
PG_FUNCTION_INFO_V1 (img_wcs_center_ra);
PG_FUNCTION_INFO_V1 (img_wcs_center_dec);

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

// helper, transform kwcs to WorldCoor. Return must by freed!
struct WorldCoor *
kwcs2wcs (struct kwcs *arg)
{
  return wcskinit (arg->naxis1, arg->naxis2, arg->ctype1, arg->ctype2,
		   arg->crpix1, arg->crpix2, arg->crval1, arg->crval2, NULL,
		   arg->cdelt1, arg->cdelt2, arg->crota, arg->equinox,
		   arg->epoch);
}

Datum
wcs_in (PG_FUNCTION_ARGS)
{
  char *imhead;
  char *cmd;

  char *next_start;
  struct kwcs *res;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  imhead = PG_GETARG_CSTRING (0);

  res = (struct kwcs *) palloc (sizeof (struct kwcs));
  memset (res, 0, sizeof (struct kwcs));

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
      ARG_CH (ctype1, 15);
      ARG_CH (ctype2, 15);
      ARG_FLOAT (crpix1);
      ARG_FLOAT (crpix2);
      ARG_FLOAT (crval1);
      ARG_FLOAT (crval2);
      ARG_FLOAT (cdelt1);
      ARG_FLOAT (cdelt2);
      ARG_FLOAT (crota);
      ARG_INT (equinox);
      ARG_FLOAT (epoch);
      else
      {
	char *warning;
	asprintf (&warning, "unknow string %s %s", cmd, arg);
	elog (NOTICE, warning);
	free (warning);
      }
      imhead = next_start;
    }
  PG_RETURN_KWCS_P (res);
}

Datum
wcs_out (PG_FUNCTION_ARGS)
{
  char *out;
  struct kwcs *arg;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);

  out = palloc (500);
  snprintf (out, 500,
	    "NAXIS1 %i NAXIS2 %i CTYPE1 %s CTYPE2 %s CRPIX1 %f CRPIX2 %f CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %i EPOCH %f",
	    arg->naxis1, arg->naxis2, arg->ctype1, arg->ctype2, arg->crpix1,
	    arg->crpix2, arg->crval1, arg->crval2, arg->cdelt1, arg->cdelt2,
	    arg->crota, arg->equinox, arg->epoch);

  PG_RETURN_CSTRING (out);
}

/*!
 * Returns true, if given ra and dec are on image.
 *
 * Use wcs2pix function from wcstools to decide so.
 *
 * @pg_arg	ra [float4]
 * @pg_arg	dec [float4
 * @pg_arg	wcs [kwcs]
 *
 * @pg_ret [boolean] true if is in, false if not, NULL on error
 */
Datum
isinwcs (PG_FUNCTION_ARGS)
{
  double ra;
  double dec;
  double pic_x, pic_y;
  struct kwcs *arg;
  struct WorldCoor *wcs;
  int ret;

  if (PG_ARGISNULL (0) || PG_ARGISNULL (1) || PG_ARGISNULL (2))
    PG_RETURN_NULL ();

  ra = PG_GETARG_FLOAT8 (0);
  dec = PG_GETARG_FLOAT8 (1);
  arg = PG_GETARG_KWCS_P (2);

  wcs = kwcs2wcs (arg);

  wcs2pix (wcs, ra, dec, &pic_x, &pic_y, &ret);

  wcsfree (wcs);

  PG_RETURN_BOOL (!ret);
}

/*!
 * Returns string with image center coordinates.
 * 
 * Use wcsrange function.
 *
 * @pg_arg	wcs [kwcs]
 *
 * @pg_ret [cstring] with image size.
 */
Datum
imgrange (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  struct WorldCoor *wcs;
  char *buffer;
  double ra1, ra2, dec1, dec2;
  text *res;
  int l;

  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  wcs = kwcs2wcs (arg);

  buffer = (char *) palloc (200);
  wcsrange (wcs, &ra1, &ra2, &dec1, &dec2);

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
  wcsfree (wcs);
  PG_RETURN_TEXT_P (res);
}

/*!
 * Returns varius WCS stuff out of kwcs
 */
Datum
img_wcs_naxis1 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_INT32 (arg->naxis1);
}

Datum
img_wcs_naxis2 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_INT32 (arg->naxis2);
}

Datum
img_wcs_ctype1 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  int l;
  text *res;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);

  l = strlen (arg->ctype1);
  l++;

  res = (text *) palloc (VARHDRSZ + l);
#ifdef RTS2_HAVE_PGSQL_SET_VARSIZE
  SET_VARSIZE (res, VARHDRSZ + l);
#else
  VARATT_SIZEP (res) = VARHDRSZ + l;
#endif
  memcpy (VARDATA (res), arg->ctype1, l);

  PG_RETURN_TEXT_P (res);
}

Datum
img_wcs_ctype2 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  int l;
  text *res;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);

  l = strlen (arg->ctype2);
  l++;

  res = (text *) palloc (VARHDRSZ + l);
#ifdef RTS2_HAVE_PGSQL_SET_VARSIZE
  SET_VARSIZE (res, VARHDRSZ + l);
#else
  VARATT_SIZEP (res) = VARHDRSZ + l;
#endif
  memcpy (VARDATA (res), arg->ctype2, l);

  PG_RETURN_TEXT_P (res);
}

Datum
img_wcs_crpix1 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->crpix1);
}

Datum
img_wcs_crpix2 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->crpix2);
}

Datum
img_wcs_crval1 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->crval1);
}

Datum
img_wcs_crval2 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->crval2);
}

Datum
img_wcs_cdelt1 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->cdelt1);
}

Datum
img_wcs_cdelt2 (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->cdelt2);
}

Datum
img_wcs_crota (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->crota);
}

Datum
img_wcs_epoch (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  PG_RETURN_FLOAT8 (arg->epoch);
}

Datum
img_wcs_center_ra (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  if (!strncmp (arg->ctype1, "RA--", 4))
    PG_RETURN_FLOAT8 (arg->crval1);
  if (!strncmp (arg->ctype2, "RA--", 4))
    PG_RETURN_FLOAT8 (arg->crval2);
  PG_RETURN_NULL ();
}

Datum
img_wcs_center_dec (PG_FUNCTION_ARGS)
{
  struct kwcs *arg;
  if (PG_ARGISNULL (0))
    PG_RETURN_NULL ();

  arg = PG_GETARG_KWCS_P (0);
  if (!strncmp (arg->ctype1, "DEC-", 4))
    PG_RETURN_FLOAT8 (arg->crval1);
  if (!strncmp (arg->ctype2, "DEC-", 4))
    PG_RETURN_FLOAT8 (arg->crval2);
  PG_RETURN_NULL ();
}
