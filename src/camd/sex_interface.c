#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>

//#define RANTL 16

#define SEX_CFG "/tmp/_sex.cfg"
#define SEX_PARAM "/tmp/_sex.param"
#define SEX_CAT "/tmp/_sex.cat"
#define SEX_BIN "sex"

#define SEXI_TEXT_SIZE 256
char sexi_text[SEXI_TEXT_SIZE];
#define SEXI_ERR(txt) \
	{\
		snprintf(sexi_text,SEXI_TEXT_SIZE,"sexi %s",txt); \
		return -1;\
	}
#define SEXI_MSG(txt) \
	{\
		snprintf(sexi_text,SEXI_TEXT_SIZE,"sexi %s",txt); \
	}

int
sexi_write_sex_cfg ()
{
  FILE *cfg;
  FILE *param;

  cfg = fopen ("/tmp/_sex.cfg", "w");
  if (!cfg)
    SEXI_ERR ("write_sex_config:cannot open " SEX_CFG " file 4w");

  fprintf (cfg,
	   "CATALOG_NAME\t" SEX_CAT "\n"
	   "CATALOG_TYPE\tASCII_HEAD\n"
	   "PARAMETERS_NAME\t" SEX_PARAM "\n"
	   "DETECT_TYPE\tCCD\n"
	   "DETECT_MINAREA\t5\n"
	   "DETECT_THRESH\t3.0\n"
	   "ANALYSIS_THRESH\t3.0\n"
	   "FILTER\tN\n"
	   "FILTER_NAME\t/tmp/default.conv\n"
	   "DEBLEND_NTHRESH\t32\n"
	   "DEBLEND_MINCONT\t0.005\n"
	   "CLEAN\tY\n"
	   "CLEAN_PARAM\t1.0\n"
	   "MASK_TYPE\tCORRECT\n"
	   "PHOT_APERTURES\t5\n"
	   "PHOT_AUTOPARAMS\t2.5, 3.5\n"
	   "SATUR_LEVEL\t50000.0\n"
	   "MAG_ZEROPOINT\t0.0\n"
	   "MAG_GAMMA\t4.0\n"
	   "GAIN\t2.8\n"
	   "PIXEL_SCALE\t1.0\n"
	   "SEEING_FWHM\t1.2\n"
	   "STARNNW_NAME\t/tmp/default.nnw\n"
	   "BACK_SIZE\t64\n"
	   "BACK_FILTERSIZE\t3\n"
	   "MEMORY_OBJSTACK\t2000\n"
	   "MEMORY_PIXSTACK\t100000\n" "MEMORY_BUFSIZE\t1024\n"
	   // "VERBOSE_TYPE\tNORMAL\n"
	   "VERBOSE_TYPE\tQUIET\n");
  fclose (cfg);

  param = fopen (SEX_PARAM, "w");
  if (!param)
    SEXI_ERR ("write_sex_config:cannot open " SEX_PARAM " file 4w");

  fprintf (param, "X_IMAGE\n" "Y_IMAGE\n" "FLUX_AUTO\n" "FWHM_IMAGE\n");
  fclose (param);

  return 0;
}

struct stardata
{
  double X, Y, F;
  double fwhm;
};

static int
sdcompare (x1, x2)
     struct stardata *x1, *x2;
{
  if (x1->fwhm < x2->fwhm)
    return 1;
  if (x1->fwhm > x2->fwhm)
    return -1;
  return 0;
}

#define BUFLEN 256
#define AAMOUNT 128

int
sexi_get_result (int *npoint, double **xpoint, double **ypoint,
		 double **flujo, double **fwhm)
{
  int a = 0, b = 0;
  char buf[BUFLEN];
  struct stardata *f = NULL;
  FILE *ff;

  ff = fopen (SEX_CAT, "r");
  if (!ff)
    SEXI_ERR ("get_result:cannot open " SEX_PARAM " file 4r");

  while (!feof (ff))
    {
      // get the line
      fgets (buf, BUFLEN, ff);
      if (buf[0] == '#')
	continue;

      // if the line is NOT a comment, allocate space for her
      if (a >= AAMOUNT * b)
	f =
	  (struct stardata *) realloc (f,
				       AAMOUNT * (++b) *
				       sizeof (struct stardata));

      // get the data (* ignores)
      sscanf (buf, " %lf %lf %lf %lf", &((f + a)->X), &((f + a)->Y),
	      &((f + a)->F), &((f + a)->fwhm));
//              if((f+a)->X > RANTL && (f+a)->Y > RANTL && (f+a)->X<(1536-RANTL) && (f+a)->Y < (1020-RANTL) )
      a++;
    }

  fclose (ff);

  qsort (f, a, sizeof (struct stardata), sdcompare);

  *xpoint = (double *) malloc (a * sizeof (double));
  *ypoint = (double *) malloc (a * sizeof (double));
  *flujo = (double *) malloc (a * sizeof (double));
  *fwhm = (double *) malloc (a * sizeof (double));

  for (b = 0; b < a; b++)
    {
      (*xpoint)[b] = f[b].X;
      (*ypoint)[b] = f[b].Y;
      (*flujo)[b] = f[b].F;
      (*fwhm)[b] = f[b].fwhm;
    }

  free (f);

  *npoint = a;

  return 0;
}

// for anyone who may want to reuse this: it's a nasty hack, read 
// it carefully before using: if modifes F[] as a "clipper"
double
sexi_sigmaclip (int *nvalues, double *F, double *Q)
{
  int N, j, x;
  double S, NN, E, tt;

  N = *nvalues;

  do
    {
      // Spocitat prumer
      S = 0;
      NN = 0;
      for (x = 0; x < N; x++)
	if (F[x] > 0)
	  {
	    S += Q[x];
	    NN++;
	  }
      S /= NN;

      // Spocitat kv. odchylku
      E = 0;
      for (x = 0; x < N; x++)
	if (F[x] > 0)
	  {
	    tt = S - Q[x];
	    E += tt * tt;
	  }
      E = sqrt (E / (NN - 1));

      // Vyhazet body mimo    
      j = 0;
      for (x = 0; x < N; x++)
	{
	  tt = S - Q[x];
	  if (F[x] > 0)
	    if ((tt > 2 * E) || (tt < -2 * E))
	      {
		F[x] = 0;
		j++;
	      }
	}
    }
  while (j);

  *nvalues = N;
  return S;
}

int
sexi_fwhm (char *fitsfile, double *fwhm)
{
  char *command;

  double *xx, *yy, *F, *Q;

  int ret, n;
  double s;

  if (((ret = sexi_write_sex_cfg ())))
    return ret;

  asprintf (&command, SEX_BIN " -c " SEX_CFG " %s", fitsfile);
  ret = system (command);
  free (command);

  if (ret == 127)		// command doesn't exist
    SEXI_ERR ("run_sex: can't run " SEX_BIN);

  if (((ret = sexi_get_result (&n, &xx, &yy, &F, &Q))))
    return ret;

  // perform some sigma-clipping to smoothen the result
  s = sexi_sigmaclip (&n, F, Q);

  // get rid of that used space
  free (xx);
  free (yy);
  free (F);
  free (Q);

  // Min star count
  if (n < 10)
    {
      SEXI_MSG ("run_sex: <10*s");

      *fwhm = 0;
    }
  else
    {
      // reusing char *command;
      asprintf (&command, "run_sex: FWHM=%f,n=%d\n", s, n);
      SEXI_MSG (command);
      free (command);

      // finally store the result
      *fwhm = s;
    }

  return 0;
}
