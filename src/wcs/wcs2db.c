/* 
 * wcs2db - convert wcs to DB insert statement
 *
 * Based on
 * File wcshead.c
 * June 19, 2002
 * By Doug Mink Harvard-Smithsonian Center for Astrophysics)
 * Send bug reports to dmink@cfa.harvard.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <fitsfile.h>
#include <wcs.h>

static void usage ();
static void ListWCS ();

static int verbose = 0;		/* verbose/debugging flag */
static int tabout = 0;		/* tab table output flag */
//static int ndec = 3;          /* maximum number of decimal places for output*/
//static int nchar = 256;               /* maximum number of characters for filename */
static int hms = 0;		/* 1 for output in hh:mm:ss dd:mm:ss */
static int version = 0;		/* If 1, print only program name and version */
static int wave = 0;		/* If 1, print first dimension limits */
static int restwave = 0;	/* If 1, print first dimension limits */

static char *RevMsg = "WCSHEAD 3.1.3, 30 August 2002, Doug Mink SAO";

int
main (ac, av)
     int ac;
     char **av;
{
  char *str;
  int readlist = 0;
  char *listfile;
  char *fn, *dn;

  /* Check for help or version command first */
  str = *(av + 1);
  if (!str || !strcmp (str, "help") || !strcmp (str, "-help"))
    usage ();
  if (!strcmp (str, "version") || !strcmp (str, "-version"))
    {
      version = 1;
      usage ();
    }

  /* crack arguments */
  for (av++; --ac > 0 && (*(str = *av) == '-' || *str == '@'); av++)
    {
      char c;
      if (*str == '@')
	str = str - 1;
      while ((c = *++str))
	switch (c)
	  {

	  case 'h':		/* hh:mm:ss output for crval, cdelt in arcsec/pix */
	    hms++;
	    break;

	  case 'n':		/* hh:mm:ss output */
	    tabout++;
	    break;

	  case 'r':		/* Print first dimension as rest wavelength, first and last values */
	    restwave++;
	    break;

	  case 't':		/* tab table output */
	    tabout++;
	    break;

	  case 'v':		/* more verbosity */
	    verbose++;
	    break;

	  case 'w':		/* Print only first dimension, first and last values */
	    wave++;
	    break;

	  case 'z':		/* Use AIPS classic WCS */
	    setdefwcs (WCS_ALT);
	    break;

	  case '@':		/* List of files to be read */
	    readlist++;
	    listfile = ++str;
	    str = str + strlen (str) - 1;
	    av++;
	    ac--;
	    break;

	  default:
	    usage ();
	    break;
	  }
    }

  /* If no arguments left, print usage */
  if (ac == 0)
    usage ();

  if (verbose)
    fprintf (stderr, "%s\n", RevMsg);

  fn = *av;
  dn = *av;

  ListWCS (fn, dn);

  return (0);
}

static void
usage ()
{
  fprintf (stderr, "%s\n", RevMsg);
  if (version)
    exit (-1);
  fprintf (stderr, "Print WCS part of FITS or IRAF image header\n");
  fprintf (stderr, "usage: wcshead [-htv] file.fit ...\n");
  fprintf (stderr, "  -h: Print CRVALs as hh:mm:ss dd:mm:ss\n");
  fprintf (stderr,
	   "  -r: Print first dimension as rest wavelength limiting values\n");
  fprintf (stderr, "  -v: Verbose\n");
  fprintf (stderr, "  -w: Print only first dimension limiting values\n");
  fprintf (stderr, "  -z: Use AIPS classic WCS subroutines\n");
  exit (1);
}

static void
ListWCS (filename)
     char *filename;		/* FITS or IRAF image file name */
{
  struct WorldCoor *wcs, *GetWCSFITS ();
  extern char *GetFITShead ();

  char *header;
  char filter[11];

  int ctime;
  float exposure;
  int obs_id;

  wcs = GetWCSFITS (filename, verbose);
  if (nowcs (wcs))
    {
      wcsfree (wcs);
      wcs = NULL;
      return;
    }

  if (wcs->ctype[0][0] == (char) 0)
    {
      wcsfree (wcs);
      wcs = NULL;
      return;
    }
  if ((header = GetFITShead (filename, verbose)) == NULL)
    {
      printf ("null header\n");
      return;
    }

  printf
    ("INSERT INTO images (img_date, img_exposure, img_temperature, img_filter, astrometry, obs_id, camera_name, mount_name, med_id, epoch_id) VALUES (nextval ('obs_id'), ");
  hgeti4 (header, "SEC", &ctime);
  printf ("%i, ", ctime);
  hgetr4 (header, "EXPOSURE", &exposure);
  printf ("%f, ", exposure * 100);
  hgetr4 (header, "CAMD_CCD", &exposure);
  printf ("%f, ", exposure * 10);
  hgets (header, "FILTER", 10, &filter);
  printf ("'%s', ", filter);
  printf
    ("'NAXIS1 %.0f NAXIS2 %.0f CTYPE1 %s CTYPE2 %s CRPIX1 %f CRPIX2 %f CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %i EPOCH %f', ",
     wcs->nxpix, wcs->nypix, wcs->ctype[0], wcs->ctype[1], wcs->crpix[0],
     wcs->crpix[1], wcs->crval[0], wcs->crval[1], wcs->cdelt[0],
     wcs->cdelt[1], wcs->rot, (int) wcs->equinox, wcs->epoch);

  hgeti4 (header, "OBSERVAT", &obs_id);
  printf (" %i ,", obs_id);
  hgets (header, "CAM_NAME", 10, &filter);
  printf ("'%s', ", filter);
  hgets (header, "TEL_NAME", 10, &filter);
  printf ("'%s', 0, '002');\n ", filter);

  free (header);

  wcsfree (wcs);
  wcs = NULL;

  return;
}

/* Feb 18 1998	New program
 * May 27 1998	Include fitsio.h instead of fitshead.h
 * Jun 24 1998	Add string lengths to ra2str() and dec2str() calls
 * Jul 10 1998	Add option to use AIPS classic WCS subroutines
 * Aug  6 1998	Change fitsio.h to fitsfile.h
 * Nov 30 1998	Add version and help commands for consistency
 *
 * Apr  7 1999	Print lines all at once instead of one variable at a time
 * Jun  3 1999	Change PrintWCS to ListWCS to avoid name conflict
 * Oct 15 1999	Free wcs using wcsfree()
 * Oct 22 1999	Drop unused variables after lint
 * Nov 30 1999	Fix declaration of ListWCS()
 *
 * Jan 28 2000	Call setdefwcs() with WCS_ALT instead of 1
 * Jun 21 2000	Add -w option to print limits for 1-d WCS
 * Aug  4 2000	Add -w option to printed option list
 *
 * Apr  8 2002	Free wcs structure if no WCS is found in file header
 * May  9 2002	Add option to print rest wavelength limits
 * May 13 2002	Set wcs pointer to NULL after freeing data structure
 * Jun 19 2002	Add verbose argument to GetWCSFITS()
 */
