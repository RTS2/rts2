#include <fitsio.h>
#include <longnam.h>
#include <stdlib.h>
#include <wcslib.h>
#include <stdio.h>
#include <jpeglib.h>
#include <wcs.h>
#include <fitsfile.h>
#include <getopt.h>
#include <math.h>

#include "logo.h"

#define PP_NEG
#define PP_BLUE

#define GAMMA 0.9

//#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

JSAMPLE *image_buffer;		/* Points to large array of image data */
int image_height = 78;		// 78
int image_width = 78;		// 78       

void
write_gray_jpeg_file (int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

//  FILE * outfile;     
  JSAMPROW row_pointer[1];
  int row_stride;
  int row_logo = 0;

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  jpeg_stdio_dest (&cinfo, stdout);

  // Four fields of the cinfo struct must be filled in: 
  cinfo.image_width = image_width;
  cinfo.image_height = image_height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
  jpeg_start_compress (&cinfo, TRUE);

  row_stride = image_width;

  while (cinfo.next_scanline < cinfo.image_height)
    {
      if (cinfo.image_height - cinfo.next_scanline <= logo_image.height)
	{
	  int i;
	  JSAMPLE *img = &image_buffer[cinfo.next_scanline * row_stride * 3];
	  for (i = 0; i < logo_image.width * 3; i++)
	    img[i] &= logo_image.pixel_data[row_logo + i];
	  row_logo += i;
	}
      /* jpeg_write_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could pass
       * more than one scanline at a time if that's more convenient.
       */
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride * 3];
      (void) jpeg_write_scanlines (&cinfo, row_pointer, 1);
    }

  jpeg_finish_compress (&cinfo);
//  fclose(outfile);
  jpeg_destroy_compress (&cinfo);
}

int
mylog (int i)
{
  int m;

  return i;

  m = (int) ((65535.0 / (pow (65535, GAMMA))) * pow (i, GAMMA));
  if (m < 0)
    m = 0;
  if (m > 65535)
    m = 65535;

  return m;
}

int
read_fits_file (char *imagen, JSAMPLE ** rim, int x, int y, int full)
{
  fitsfile *pim = NULL;
  int status = 0, i, j, k, m, n, w, nfound, anynull;
  long naxes[2];
  long npixels, fpixel;
  float nullval;
  unsigned short *im = 0;
  int low, med, hig, ds;
  int hist[65536];

  // OPEN THE IMAGE FILE
  if (ffopen (&pim, imagen, READONLY, &status))
    {
      fprintf (stderr, "%s is not accessible\n", imagen);
      goto do_error;
    }

  // GET IMAGE DATA 
  if (fits_read_keys_lng (pim, "NAXIS", 1, 2, naxes, &nfound, &status))
    goto do_error;
  if (full)
    {
      image_width = naxes[0];
      image_height = naxes[1];
    }

  if (x == -1)
    x = image_width / 2;
  if (y == -1)
    y = image_height / 2;

  *rim =
    (JSAMPLE *) malloc (image_width * image_height * sizeof (JSAMPLE) * 3);

  npixels = naxes[0] * naxes[1];
  fpixel = 1;
  nullval = 0;
  im = (unsigned short *) malloc (npixels * sizeof (unsigned short));
  fits_read_img (pim, TUSHORT, fpixel, npixels, &nullval, im, &anynull,
		 &status);

  fprintf (stderr, "axes: x:%ld, y:%ld\n", naxes[0], naxes[1]);
  fflush (stdout);

  // histogram build
  for (i = 0; i < 65536; i++)
    hist[i] = 0;

  ds = 0;
  k = 0;
  for (j = y; j < y + image_height; j++)
    for (i = x; i < x + image_width; i++)
      {
	m = mylog (im[i + naxes[0] * j]);

	if (m < 32768)
	  {
	    hist[m]++;
	    ds++;
	  }
      }

  low = hig = med = 0;
  j = 0;
  for (i = 0; i < 65536; i++)
    {
      j += hist[i];
      if ((!low) && (((float) j / (float) ds) > PP_LOW))
	low = i;
#ifdef PP_MED
      if ((!med) && (((float) j / (float) ds) > PP_MED))
	med = i;
#endif
      if ((!hig) && (((float) j / (float) ds) > PP_HIG))
	hig = i;
    }
  fprintf (stderr, "levels: low:%d, med:%d, hi:%d\n", low, med, hig);

  k = 0;
  for (j = 0; j < naxes[1] && j < npixels; j++)
    if ((j >= y) && (j < y + image_height))
      {
	for (i = 0, w = 0; i < naxes[0]; i++)

	  if ((i >= x) && (i < x + image_width))
	    {
	      w++;
	      m = mylog (im[i + naxes[0] * j]);

	      if (m < low)
		n = 0;
	      else if (m > hig)
		n = 511;
#ifdef PP_MED
	      else if (m < med)
		n = 31 * ((float) im[i + naxes[0] * j] - low) / (med - low);
	      else
		n =
		  32 + 480 * ((float) im[i + naxes[0] * j] - med) / (hig -
								     med);
#else
	      else
		n = 511 * ((float) im[i + naxes[0] * j] - low) / (hig - low);
#endif
#ifdef PP_NEG
	      n = 511 - n;
#endif
#ifdef PP_BLUE
	      (*rim)[k++] = ((n - 256) > 0) ? n - 256 : 0;
	      (*rim)[k++] = n / 2;
	      (*rim)[k++] = (n < 256) ? n : 255;
#else
	      (*rim)[k++] = (n < 256) ? n : 255;
	      (*rim)[k++] = n / 2;
	      (*rim)[k++] = ((n - 256) > 0) ? n - 256 : 0;
#endif
	    }
	k += image_width - w;
      }


  if (status)
    fits_report_error (stderr, status);

  status = 0;
  if (pim)
    fits_close_file (pim, &status);
  if (status)
    fits_report_error (stderr, status);

  if (im)
    free (im);

  return status;
do_error:
  exit (5);
}

struct WorldCoor *
WCS_init (char *inputfile)
{
  int lhead, nbfits;
  struct WorldCoor *wcsim;
  char *header;

  if ((header = fitsrhead (inputfile, &lhead, &nbfits)) == NULL)
    {
      fprintf (stderr, "Cannot read the header from %s \n", inputfile);
      return 0;
    }

  if ((wcsim = wcsinit (header)) == NULL)
    {
      fprintf (stderr, "Can't get the header stuff\n");
      return 0;
    }
  return wcsim;
}

int
main (int argc, char **argv)
{
  struct WorldCoor *wcsim = 0;	// World coordinate system structure 

  int off;
  double x, y;
  double a, d;

  int c;

/*  while (1)
  {
	c = getopt



  } */

  if ((argc != 2) && (argc != 3) && (argc != 4))
    {
      fprintf (stderr,
	       "Usage: fits2jpeg image.fits [X Y|full] > image.jpg\n");
      exit (1);
    }

  if (!
      (image_buffer =
       (JSAMPLE *) malloc (image_width * image_height * sizeof (JSAMPLE) *
			   3)))
    {
      fprintf (stderr, "Can't have a buffer for an image\n");
      exit (2);
    }

  if (argc == 4)
    {
      if (!(wcsim = WCS_init (argv[1])))
	{
	  fprintf (stderr, "Can't initialize WCS\n");
	  exit (3);
	}

      a = atof (argv[2]);
      d = atof (argv[3]);

      wcsininit (wcsim, "FK5");
      wcs2pix (wcsim, a, d, &x, &y, &off);

      fprintf (stderr, "area: a=%f d=%f -> x=%f y=%f\n", a, d, x, y);
      if (off)
	{
	  fprintf (stderr, "IT'S OFF\n");
	  exit (4);
	}


//              x=atof(argv[3]);
//              y=atof(argv[4]);

      x -= image_width / 2;
      y -= image_height / 2;

      if (x < 0)
	x = 0;
      if (y < 0)
	y = 0;
    }
  else if (argc != 3)
    x = y = -1;
  else
    x = y = 0;

  read_fits_file (argv[1], &image_buffer, x, y, argc == 3);
  write_gray_jpeg_file (85);

  exit (0);
}
