#include <fitsio.h>
#include <stdio.h>
#include <stdlib.h>

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

  int nval;

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
  printf ("INSERT INTO images"
	  "(img_id, img_date, img_exposure, img_temperature, img_filter, astrometry, "
	  "obs_id, camera_name, mount_name, med_id, epoch_id) VALUES (nextval ('img_id'), "
	  "%ld, %.0f, %.0f, '%s', 'NAXIS1 %ld NAXIS2 %ld CTYPE1 %s CTYPE2 %s CRPIX1 %ld CRPIX2 %ld "
	  "CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %f EPOCH %f', %ld, '%s', '%s',"
	  "0, '00T');\n", img_date, img_exposure * 100, img_temperature * 10,
	  img_filter, naxis[0], naxis[1], ctype[0], ctype[1], crpix[0],
	  crpix[1], crval[0], crval[1], cdelt[0], cdelt[1],
	  (crota[0] + crota[1]) / 2.0, eqin, epoch, obs_id, camera_name,
	  mount_name);
err:
  if (status)
    fits_report_error (stderr, status);
  fits_close_file (fptr, &status);
  return status;
}
