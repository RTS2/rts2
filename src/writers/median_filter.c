#include <stdio.h>

#include "fitsio.h"

void
printerror (int status)
{
  if (status)
    {
      fits_report_error (stderr, status);	/* print error report */
      exit (status);		/* terminate the program, returning error status */
    }
  return;
}

int
main (int argc, char **argv)
{
  fitsfile *fptr;		/* pointer to the FITS file, defined in fitsio.h */
  int status, nfound, anynull;
  long naxes[2], npixels;

  unsigned short *data, *filter_top, nullval;
  int last_val, curr_val, next_val;

  char *filename = argv[1];	/* name of existing FITS file   */

  if (argc != 2)
    {
      printf ("Usage: median_filter <fits>\n");
      exit (EXIT_FAILURE);
    }

  status = 0;

  if (fits_open_file (&fptr, filename, READWRITE, &status))
    printerror (status);

  /* read the NAXIS1 and NAXIS2 keyword to get image size */
  if (fits_read_keys_lng (fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
    printerror (status);

  npixels = naxes[0] * naxes[1];	/* number of pixels in the image */
  nullval = 0;			/* don't check for null values in the image */

  data = (unsigned short *) malloc (npixels * sizeof (unsigned short));

  if (fits_read_img (fptr, TUSHORT, 1, npixels, &nullval,
		     data, &anynull, &status))
    printerror (status);

  filter_top = data;

  last_val = *filter_top;
  filter_top++;
  curr_val = *filter_top;
  filter_top++;

  while (filter_top - data < npixels)
    {
      next_val = *filter_top;
      if ((curr_val - last_val > 400 && curr_val - next_val > 400)
	  || (curr_val - last_val < -400 && curr_val - next_val < -400))
	*(filter_top - 1) = (last_val + next_val) / 2;
      last_val = curr_val;
      curr_val = next_val;
      filter_top++;
    }

  if (fits_write_img (fptr, TUSHORT, 1, npixels, data, &status))
    printerror (status);

  free (data);

  if (fits_close_file (fptr, &status))
    printerror (status);

  return EXIT_SUCCESS;
}
