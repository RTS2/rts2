#define __GNU_SOURCE

#include <fitsio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils/rts2config.h"

#define test_nval(name,count)	if (count != nval) { fprintf (stderr, "Invalid number of values in %s (expecting %i, got %i)\n",\
				name, count, nval); goto err; }

#define read_long(name,val)	if (fits_read_key_lng (fptr, name, val, NULL, &status)) goto err;
#define read_longs(name,from,count,val)	if (fits_read_keys_lng (fptr, name, from, count, val, &nval, &status)) \
				goto err; test_nval (name, count);
#define read_double(name,val)	if (fits_read_key_dbl (fptr, name, val, NULL, &status)) goto err;
#define read_doubles(name,from,count,val)  if (fits_read_keys_dbl (fptr, name, from, count, val, &nval, &status)) \
				goto err; test_nval (name, count);
#define read_string(name,val)	if (fits_read_key_str (fptr, name, val, NULL, &status)) goto err;
#define read_strings(name,from,count,val)  if (fits_read_keys_str (fptr, name, from, count, val, &nval, &status)) \
				goto err; test_nval (name, count);

int
main (int argc, char **argv)
{
  fitsfile *fptr;

  int status;
  unsigned long img_date;
  double img_exposure;
  double img_temperature;
  char img_filter[11];
  long naxis[2];
  char **ctype;
  long crpix[2];
  double crval[2];
  double cdelt[2];
  double crota[2];
  double eqin;
  double epoch;
  long obs_id;
  char camera_name[10];
  char mount_name[10];

  char *obs_epoch;

  int nval;

  int c;

  obs_epoch = "00T";

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"epoch", 1, 0, 'e'},
	{"help", 0, 0, 'h'}
      };

      c = getopt_long (argc, argv, "e:h", long_option, NULL);

      if (c == -1)
	break;
      switch (c)
	{
	case 'e':
	  obs_epoch = optarg;
	  break;
	case 'h':
	  printf
	    ("Options:\n\tepoch|e <epoch>\t\tobservation epoch\n"
	     "\t\t(that one will be written to the database; it overwrites configuration options)\n"
	     "\thelp|h\t\t\tprint that help\n");
	  exit (0);

	}
    }

  status = 0;
  if (argc == 1)
    {
      fprintf (stderr, "No file to open!\n");
      exit (EXIT_FAILURE);
    }
  if (fits_open_file (&fptr, argv[1], READONLY, &status))
    goto err;
  if (fits_read_key_lng (fptr, "CTIME", &img_date, NULL, &status))
    {
      read_long ("SEC", &img_date);
      status = 0;
    }
  read_double ("EXPOSURE", &img_exposure);
  if (fits_read_key_dbl (fptr, "CAM_TEMP", &img_temperature, NULL, &status))
    {
      status = 0;
      read_double ("CAMD_CCD", &img_temperature);
    }
  read_string ("FILTER", img_filter);
  read_longs ("NAXIS", 1, 2, naxis);
  ctype = malloc (sizeof (char *) * 2);
  ctype[0] = (char *) malloc (9);
  ctype[1] = (char *) malloc (9);
  read_strings ("CTYPE", 1, 2, ctype);
  read_doubles ("CRVAL", 1, 2, crval);
  read_longs ("CRPIX", 1, 2, crpix);
  read_doubles ("CDELT", 1, 2, cdelt);
  read_doubles ("CROTA", 1, 2, crota);
  read_double ("EQUINOX", &eqin);
  read_double ("EPOCH", &epoch);
  read_long ("OBSERVAT", &obs_id);
  read_string ("CAM_NAME", camera_name);
  read_string ("TEL_NAME", mount_name);
  printf ("INSERT INTO images ("
	  "\timg_id\n,"
	  "\timg_date\n,"
	  "\timg_exposure,\n"
	  "\timg_temperature,\n"
	  "\timg_filter,\n"
	  "\tastrometry,\n"
	  "\tobs_id,\n"
	  "\tcamera_name,\n"
	  "\tmount_name,\n"
	  "\tmed_id,\n"
	  "\tepoch_id)\n"
	  "VALUES ("
	  "\tnextval ('img_id'),\n"
	  "\tabstime (%ld),\n"
	  "\t%.0f,\n"
	  "\t%.0f,\n"
	  "\t'%s',\n"
	  "\t'NAXIS1 %ld NAXIS2 %ld CTYPE1 %s CTYPE2 %s CRPIX1 %ld CRPIX2 %ld "
	  "CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %f EPOCH %f',\n"
	  "\t%ld,\n"
	  "\t'%s',\n"
	  "\t'%s',\n"
	  "\t0,\n"
	  "\t'%s'\n"
	  ");\n",
	  img_date,
	  img_exposure * 100,
	  img_temperature * 10,
	  img_filter,
	  naxis[0],
	  naxis[1],
	  ctype[0],
	  ctype[1],
	  crpix[0],
	  crpix[1],
	  crval[0],
	  crval[1],
	  cdelt[0],
	  cdelt[1],
	  (crota[0] + crota[1]) / 2.0,
	  eqin, epoch, obs_id, camera_name, mount_name, obs_epoch);
err:
  if (status)
    fits_report_error (stderr, status);
  fits_close_file (fptr, &status);
  return status;
}
