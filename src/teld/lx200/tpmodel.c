/*
 * Implementation of the TPOINT modeled terms. 
 * YET IMPLEMENTED: ME,MA, IH,ID, CH,NP, PDD,PHH
 * TODO: HCES,HCEC, DCES,DCEC, FO,TF (BART)
 * 	 A1D,A1H, FLOP (BOOTES1B) 
 */

#include <stdio.h>
#include <math.h>
#include <libnova.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define sgn(i) ((i)<0?-1:1)

#ifdef TUNE
#warning This TUNE is only a debugging option: not for regular use!
#endif

/* Number of terms to be allocated */
#define TPTERMS 30
/* Model name/path */
#define MODEL_PATH "/home/BART/tpmodel.dat"

double
in180 (double x)
{
  for (; x > 180; x -= 360);
  for (; x < -180; x += 360);
  return x;
}

double
in360 (double x)
{
  for (; x > 360; x -= 360);
  for (; x < 0; x += 360);
  return x;
}

double
get_refraction (double altitude)
{
  double R;

  //altitude = deg_to_rad (altitude);

  /* eq. 5.27 (Telescope Control,M.Trueblood & R.M. Genet) */
  R = 1.02 /
    tan (deg_to_rad (altitude + 10.3 / (altitude + 5.11) + 0.0019279));

  // for gnuplot (in degrees)
  // R(a) = 1.02 / tan( (a + 10.3 / (a + 5.11) + 0.0019279) / 57.2958 ) / 60

  /* take into account of atm press and temp */
  //R *= ((atm_pres / 1010) * (283 / (273 + temp)));
  R *= ((1013.6 / 1010) * (283 / (273 + 10)));

  /* convert from arcminutes to degrees */
  R /= 60.0;

  return (R);
}

char *
scRMS (double rms)		// autoscale rms
{
  static char buf[32];

  if (fabs (rms) < 0.3)
    sprintf (buf, "%0.3f\"", rms);
  else if (fabs (rms) < 3.0)
    sprintf (buf, "%0.2f\"", rms);
  else if (fabs (rms) < 30.)
    sprintf (buf, "%.1f\"", rms);
  else if (fabs (rms) < 60.)
    sprintf (buf, "%.0f\"", rms);
  else if (fabs (rms) < 180)
    sprintf (buf, "%0.2f\'", rms / 60);
  else if (fabs (rms) < 1800)
    sprintf (buf, "%0.1f\'", rms / 60);
  else if (fabs (rms) < 3600)
    sprintf (buf, "%0.0f\'", rms / 60);
  else
    sprintf (buf, "%0.2fo", rms / 3600);

  return buf;
}


struct tpterm
{
  char cp;			// chained/parallel flag
  char ff;			// fixed/floating flag
  char name[9];			// term name (%8s+NULL)
  double value;			// coefficient value
  double sigma;			// coefficient sigma
  void (*apply) (struct ln_equ_posn * in, struct ln_equ_posn * out,
		 double corr, double a1, double a2);
};

struct tpmodel
{
  char caption[81];		// Model description: 80 chars + NULL
  char method;			// method: T or S
  int num;			// Number of active observations
  double rms;			// sky RMS (arcseconds)
  double refA;			// refraction constant A (arcseconds)
  double refB;			// refraction constant B (arcseconds)
  int terms, termrefs;		// number of terms used
  // TERMS
  struct tpterm *term[TPTERMS];
  // May happen there will be more of them, but...
} modeli, *model = &modeli;

// double to sexagesimal conversion: "sDD MM SS", s = 1 if sign's required
char *
d2s (double d)
{
  static char buffer[15];
  double t, dd, dm, ds;
  if (d > 0)
    t = d;
  else
    t = -d;
  dd = floor (t);
  t = 60 * (t - dd);
  dm = floor (t);
  t = 60 * (t - dm);
  ds = t;
  if (d < 0)
    dd = -dd;
  sprintf (buffer, "%+2.0f %02.0f %05.2f", dd, dm, ds);
  return buffer;
}

char *
r2s (double d)
{
  static char buffer[15];
  double t, dd, dm, ds;
  if (d > 0)
    t = d;
  else
    t = -d;
  dd = floor (t);
  t = 60 * (t - dd);
  dm = floor (t);
  t = 60 * (t - dm);
  ds = t;
  if (d < 0)
    dd = -dd;
  sprintf (buffer, "%3.0f %02.0f %05.2f", dd, dm, ds);
  return buffer;
}

// Polar Axis Misalignment in Elevation
// status: OK
void
applyME (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  double h0, d0, h1, d1, M, N;

  h0 = deg_to_rad (in180 (in->ra));
  d0 = deg_to_rad (in180 (in->dec));

  N = asin (sin (h0) * cos (d0));
  M = asin (sin (d0) / cos (N));

  if ((h0 > (M_PI / 2)) || (h0 < (-M_PI / 2)))
    {
      if (M > 0)
	M = M_PI - M;
      else
	M = -M_PI + M;
    }

  M = M - deg_to_rad (corr / 3600);

  if (M > M_PI)
    M -= 2 * M_PI;
  if (M < -M_PI)
    M += 2 * M_PI;

  d1 = asin (sin (M) * cos (N));
  h1 = asin (sin (N) / cos (d1));

  if (M > (M_PI / 2))
    h1 = M_PI - h1;
  if (M < (-M_PI / 2))
    h1 = -M_PI + h1;

  if (h1 > M_PI)
    h1 -= 2 * M_PI;
  if (h1 < -M_PI)
    h1 += 2 * M_PI;

  out->ra = rad_to_deg (h1);
  out->dec = rad_to_deg (d1);
}

// Polar Axis Misalignment in Azimuth
// status: OK
void
applyMA (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  double d, h;

  d = in->dec - (corr / 3600) * sin (deg_to_rad (in->ra));	// Polar Axis Misalignment in Azimuth
  h =
    in->ra +
    (corr / 3600) * cos (deg_to_rad (in->ra)) * tan (deg_to_rad (in->dec));

  out->ra = h;
  out->dec = d;
}

// Index Error in Hour Angle 
// status: OK
void
applyIH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  // Add a zero point to the hour angle
  out->ra = in180 (in->ra - corr / 3600);
  // No change for declination
  out->dec = in->dec;
}

// Index Error in Declination
// status: OK
void
applyID (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  // Add a zero point to the declination
  out->dec = in180 (in->dec - corr / 3600);
  // No change for hour angle
  out->ra = in->ra;
}

// East-West Collimation Error
// status: OK
void
applyCH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  out->ra = in->ra - (corr / 3600) / cos (deg_to_rad (in->dec));
  out->dec = in->dec;
}

// HA/Dec Non-perpendicularity
// status: chyba
void
applyNP (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2)
{
  out->ra = in->ra - (corr / 3600) * tan (deg_to_rad (in->ra));
  out->dec = in->dec;
}

// Step size in h (for Paramount, where it's unsure)
void
applyPHH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2)
{
  out->ra =
    in->ra - rad_to_deg (deg_to_rad (corr / 3600) * deg_to_rad (in->ra));
  out->dec = in->dec;
}

// Step size in \delta (for Paramount, where it's unsure)
void
applyPDD (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2)
{
  out->dec =
    in->dec - rad_to_deg (deg_to_rad (corr / 3600) * deg_to_rad (in->dec));
  out->ra = in->ra;
}

//    h -= TP_A1H / 3600 * (double) flip;
//    d -= TP_A1D / 3600 * (double) flip;

void
applyProperMotion (struct ln_equ_posn *in, struct ln_equ_posn *out,
		   struct ln_equ_posn *pm, double JD)
{
  struct ln_equ_posn result;

  get_equ_pm (in, pm, JD, &result);

  out->ra = result.ra;
  out->dec = result.dec;
}

void
applyAberation (struct ln_equ_posn *in, struct ln_equ_posn *out, double JD)
{
  struct ln_equ_posn result;

  get_equ_aber (in, JD, &result);

  out->ra = result.ra;
  out->dec = result.dec;
}


void
applyPrecession (struct ln_equ_posn *in, struct ln_equ_posn *out, double JD)
{
  struct ln_equ_posn result;

  get_equ_prec (in, JD, &result);

  out->ra = result.ra;
  out->dec = result.dec;
}

void
applyRefraction (struct ln_equ_posn *in, struct ln_equ_posn *out,
		 struct ln_lnlat_posn *obs, double JD)
{
  struct ln_equ_posn result;
  struct ln_hrz_posn hrz;
  double ref;

  get_hrz_from_equ (in, obs, JD, &hrz);

  ref = get_refraction (hrz.alt);	//, 1010.0, 10.0);

  hrz.alt += ref;
  get_equ_from_hrz (&hrz, obs, JD, &result);

  result.ra = in360 (result.ra);
  result.dec = in180 (result.dec);

  out->ra = result.ra;
  out->dec = result.dec;
}

struct tpa_termref
{
  void (*apply) (struct ln_equ_posn * in, struct ln_equ_posn * out,
		 double corr, double a1, double a2);
  char name[8];
} termref[] =
{
  {
  applyME, "ME"},
  {
  applyMA, "MA"},
  {
  applyIH, "IH"},
  {
  applyID, "ID"},
  {
  applyCH, "CH"},
  {
  applyNP, "NP"},
  {
  applyPHH, "PHH"},
  {
  applyPDD, "PDD"}
};

int
get_model (char *filename)
{
  FILE *mf;
  char buffer[256];
  int l, i = 0, j;

  mf = fopen (filename, "r");

  if (mf == NULL)
    return -1;

  fgets (buffer, 256, mf);
  l = strlen (buffer);
  if (l > 80)
    l = 80;
  strncpy (model->caption, buffer, l - 1);
  model->caption[l] = 0;

  model->termrefs = sizeof (termref) / sizeof (struct tpa_termref);

#if TUNE
  fprintf (stderr, "[%s]\n", model->caption);
#endif

  fgets (buffer, 256, mf);
  sscanf (buffer, "%c%5d%9lf%9lf%9lf", &model->method, &model->num,
	  &model->rms, &model->refA, &model->refB);

#if TUNE
  fprintf (stderr, "%d points with RMS %s\n", model->num, scRMS (model->rms));
#endif

  while (!feof (mf))
    {
      fgets (buffer, 256, mf);
      if (!strncmp (buffer, "END", 3))
	break;

      model->term[i] = malloc (sizeof (struct tpterm));

      sscanf (buffer, "%c%c%8s%10lf%12lf",
	      &(model->term[i]->cp), &(model->term[i]->ff),
	      model->term[i]->name, &(model->term[i]->value),
	      &(model->term[i]->sigma));

      /*          fprintf(stderr, "%-8s%7s %.2f%% (%c%c)\n", model->term[i]->name, 
         scRMS(model->term[i]->value), 
         100*model->term[i]->sigma/model->term[i]->value,
         model->term[i]->cp,model->term[i]->ff);
       */
      i++;
      if (i > TPTERMS)
	{
	  fprintf (stderr,
		   "Too many terms for me, change the value of TPTERMS(=%d) in source and recompile.\n",
		   TPTERMS);
	  break;
	}
    }

  model->terms = i;

  for (i = 0; i < model->terms; i++)
    {
      model->term[i]->apply = NULL;

      for (j = 0; j < model->termrefs; j++)
	{
	  if (!strcmp (termref[j].name, model->term[i]->name))
	    {
	      model->term[i]->apply = termref[j].apply;
	      break;
	    }
	}
    }


  return 0;
}

#if TUNE
void
bonz (struct ln_equ_posn *pos)
{
  printf ("[%f,%f]\n", pos->ra, pos->dec);
}
#else
#define bonz(a)
#endif

int
tpoint_correction (struct ln_equ_posn *mean_pos,	/* mean pos of the object to go */
		   struct ln_equ_posn *proper_motion,	/* proper motion of the object: may be NULL */
		   struct ln_lnlat_posn *obs,	/* observer location. if NULL: no refraction */
		   double JD,	/* time */
		   struct ln_equ_posn *tel_pos	/* return: coords corrected for the model */
  )
{
  int i, ret = 0;
  double Q;

  Q = get_apparent_sidereal_time (JD) * 360.0 / 24.0;

  get_model (MODEL_PATH);

  tel_pos->ra = mean_pos->ra;
  tel_pos->dec = mean_pos->dec;

  bonz (tel_pos);

  if (proper_motion)		/* pm may be omitted */
    applyProperMotion (tel_pos, tel_pos, proper_motion, JD);
  applyAberation (tel_pos, tel_pos, JD);
  applyPrecession (tel_pos, tel_pos, JD);
  if (obs)
    applyRefraction (tel_pos, tel_pos, obs, JD);

  bonz (tel_pos);

  tel_pos->ra = in180 (Q - tel_pos->ra);	// make ha

  for (i = 0; i < model->terms; i++)
    if (model->term[i]->apply)
      model->term[i]->apply (tel_pos, tel_pos, model->term[i]->value, 0, 0);

  tel_pos->ra = in360 (Q - tel_pos->ra);	// make ra from ha

  bonz (tel_pos);

  return ret;
}

struct ln_lnlat_posn lazy_observer = {
//    +6.732778,
//    37.104444
  -14.783639,
  +49.910556
};

/* lazy func: correct me there coords simply (no proper motion, now!, in place) */
// everything in degrees */
void
tpoint_apply_now (double *ra, double *dec)
{
  struct ln_equ_posn equ;
  struct ln_equ_posn mean_pos;
  double JD;
  int ret = 0;

  mean_pos.ra = *ra;
  mean_pos.dec = *dec;

  JD = get_julian_from_sys ();

  ret = tpoint_correction (&mean_pos, NULL, &lazy_observer, JD, &equ);

  // If error happened do not modify values
  if (ret)
    return;
  *ra = equ.ra;
  *dec = equ.dec;
}

#ifdef TUNE

int
main ()
{
  int i;
  double r, d = 0;
  for (i = 0; i < 15; i++)
    {
      r = i * 24.0;
      d = 0;
      printf ("<%f,%f>\n", r, d);
      tpoint_apply_now (&r, &d);
      printf ("<%f,%f>\n\n", r, d);
    }

}

#endif
