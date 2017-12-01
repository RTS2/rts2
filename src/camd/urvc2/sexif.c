#include <stdio.h>
#ifdef RTS2_HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdlib.h>
#include <math.h>

#define RANTL 16

void
write_sex_cfg ()
{
  FILE *cfg;
  FILE *param;

  cfg = fopen ("_sex.cfg", "w");

  fprintf (cfg, "CATALOG_NAME\t%s\n", "_sex.cat");
  fprintf (cfg, "CATALOG_TYPE\tASCII_HEAD\n");
  fprintf (cfg, "PARAMETERS_NAME\t_sex.param\n");
  fprintf (cfg, "DETECT_TYPE\tCCD\n");
  fprintf (cfg, "DETECT_MINAREA\t%d\n", 5);
  fprintf (cfg, "DETECT_THRESH\t%.1f\n", 3.0);
  fprintf (cfg, "ANALYSIS_THRESH\t%.1f\n", 3.0);
  fprintf (cfg, "FILTER\tN\n");
  fprintf (cfg, "FILTER_NAME\tdefault.conv\n");
  fprintf (cfg, "DEBLEND_NTHRESH\t32\n");
  fprintf (cfg, "DEBLEND_MINCONT\t0.005\n");
  fprintf (cfg, "CLEAN\tY\n");
  fprintf (cfg, "CLEAN_PARAM\t1.0\n");
  fprintf (cfg, "MASK_TYPE\tCORRECT\n");
  fprintf (cfg, "PHOT_APERTURES\t5\n");
  fprintf (cfg, "PHOT_AUTOPARAMS\t2.5, 3.5\n");
  fprintf (cfg, "SATUR_LEVEL\t50000.0\n");
  fprintf (cfg, "MAG_ZEROPOINT\t0.0\n");
  fprintf (cfg, "MAG_GAMMA\t4.0\n");
  fprintf (cfg, "GAIN\t2.8\n");
  fprintf (cfg, "PIXEL_SCALE\t1.0\n");
  fprintf (cfg, "SEEING_FWHM\t1.2\n");
  fprintf (cfg, "STARNNW_NAME\tdefault.nnw\n");
  fprintf (cfg, "BACK_SIZE\t64\n");
  fprintf (cfg, "BACK_FILTERSIZE\t3\n");
  fprintf (cfg, "MEMORY_OBJSTACK\t2000\n");
  fprintf (cfg, "MEMORY_PIXSTACK\t100000\n");
  fprintf (cfg, "MEMORY_BUFSIZE\t1024\n");
  //fprintf(cfg,"VERBOSE_TYPE\tNORMAL\n");
  fprintf (cfg, "VERBOSE_TYPE\tQUIET\n");

  fclose (cfg);

  param = fopen ("_sex.param", "w");
  fprintf (param, "X_IMAGE\n");
  fprintf (param, "Y_IMAGE\n");
  fprintf (param, "FLUX_AUTO\n");
  fprintf (param, "FWHM_IMAGE\n");
  fclose (param);
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
get_result (int *npoint, double **xpoint, double **ypoint, double **flujo,
	    double **fwhm)
{
  int a = 0, b = 0;
  char buf[BUFLEN], *sex;
  struct stardata *f = NULL;
  FILE *ff;

  sex = "_sex.cat";

  // pokud pouzijes tohle, odkomentuj free na konci rutiny
  //asprintf(&sex, "%s.sex", arg);

  if (NULL == (ff = fopen (sex, "r")))
    return -1;
//              fatal(254, "Can't open %s\n", sex);

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

  //free(sex);
  free (f);

  *npoint = a;
  fprintf (stderr, "%s: %d records\n", sex, a);
  fflush (stderr);
  return a;
}

double
run_sex (char *fitsfile)
{
  char command[256];

  int N, j, x;
  double *xx, *yy, *F, *Q;
  double S, NN, E, tt;

  write_sex_cfg ();
  sprintf (command, "sex -c _sex.cfg %s", fitsfile);
  system (command);

  get_result (&N, &xx, &yy, &F, &Q);

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


  // Min star count
  if (N > 10)
    return S;
  else
    return -1.0;
}
