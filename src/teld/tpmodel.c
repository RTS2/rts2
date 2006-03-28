/*
 * Implementation of the TPOINT modeled terms. 
 * YET IMPLEMENTED: ME,MA, IH,ID, CH,NP, PDD,PHH
 * 	 	    A1D, A1H
 * TODO: HCES,HCEC, DCES,DCEC, FO,TF (BART)
 * 	 	    FLOP (BOOTES1B), 
 */

#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define sgn(i) ((i)<0?-1:1)

#define DEBUG

#ifdef TUNE
#warning This TUNE is only a debugging option: not for regular use!
#define DEBUG
#endif

/* Number of terms to be allocated */
#define TPTERMS 30
/* Model name/path */
#define MODEL_PATH_N "/etc/rts2/b1b-n.dat"
#define MODEL_PATH_F "/etc/rts2/b1b-f.dat"

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

  //altitude = ln_deg_to_rad (altitude);

  /* eq. 5.27 (Telescope Control,M.Trueblood & R.M. Genet) */
  R = 1.02 /
    tan (ln_deg_to_rad (altitude + 10.3 / (altitude + 5.11) + 0.0019279));

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
		 double corr, double a1, double a2, double phi);
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
	 double a1, double a2, double phi)
{
  double h0, d0, h1, d1, M, N;

  h0 = ln_deg_to_rad (in180 (in->ra));
  d0 = ln_deg_to_rad (in180 (in->dec));

  N = asin (sin (h0) * cos (d0));
  M = asin (sin (d0) / cos (N));

  if ((h0 > (M_PI / 2)) || (h0 < (-M_PI / 2)))
    {
      if (M > 0)
	M = M_PI - M;
      else
	M = -M_PI + M;
    }

  M = M + ln_deg_to_rad (corr / 3600);

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

  out->ra = ln_rad_to_deg (h1);
  out->dec = ln_rad_to_deg (d1);
}

// Polar Axis Misalignment in Azimuth
// status: OK
void
applyMA (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  double d, h;

  d = in->dec + (corr / 3600) * sin (ln_deg_to_rad (in->ra));
  h =
    in->ra -
    (corr / 3600) * cos (ln_deg_to_rad (in->ra)) *
    tan (ln_deg_to_rad (in->dec));

  out->ra = h;
  out->dec = d;
}

// Index Error in Hour Angle 
// status: OK
void
applyIH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  // Add a zero point to the hour angle
  out->ra = in180 (in->ra + corr / 3600);
  // No change for declination
  out->dec = in->dec;
}

// Index Error in Declination
// status: OK
void
applyID (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  // Add a zero point to the declination
  out->dec = in180 (in->dec + corr / 3600);
  // No change for hour angle
  out->ra = in->ra;
}

// East-West Collimation Error
// status: OK -- patched with (-1*a1) to behave well
void
applyCH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  // out->ra = in->ra - (-1*a1) * (corr / 3600) / cos (ln_deg_to_rad (in->dec));
  out->ra = in->ra + (corr / 3600) / cos (ln_deg_to_rad (in->dec));
  printf ("\033[14;3HCH:"
	  "in->ra=%f "
	  "corr=%f "
	  "in->dec=%f "
	  "1/cos(in->dec)=%f "
	  "fix=%f",
	  in->ra,
	  corr / 3600,
	  in->dec,
	  1 / cos (ln_deg_to_rad (in->dec)),
	  +(corr / 3600) / cos (ln_deg_to_rad (in->dec)));

  out->dec = in->dec;
}

// HA/Dec Non-perpendicularity
// status: used to fail, now testing
void
applyNP (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  out->ra = in->ra + (corr / 3600) * tan (ln_deg_to_rad (in->dec));
  out->dec = in->dec;
}

// FO: Fork Flexure
// status: testing
void
applyFO (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  out->dec = in->dec + (corr / 3600) * cos (ln_deg_to_rad (in->ra));
  out->ra = in->ra;
}

// HPSH4...? Zvrhlost! Nicmene para ji potrebuje, takze casem...
void
applyHPSH4 (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	    double a1, double a2, double phi)
{
}

// Step size in h (for Paramount, where it's unsure)
// status: ok
void
applyPHH (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2, double phi)
{
  out->ra =
    in->ra +
    ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * ln_deg_to_rad (in->ra));
  out->dec = in->dec;
}

// Step size in \delta (for Paramount, where it's unsure)
// status: ok
void
applyPDD (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2, double phi)
{
  out->dec =
    in->dec +
    ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * ln_deg_to_rad (in->dec));
  //out->dec = in->dec - ln_rad_to_deg ( (corr / 3600) * ln_deg_to_rad (in->dec));
  out->ra = in->ra;
}

void
applyHCEC (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	   double a1, double a2, double phi)
{
  out->ra = in->ra + (corr / 3600) * cos (ln_deg_to_rad (in->ra));
  out->dec = in->dec;
}

void
applyHCES (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	   double a1, double a2, double phi)
{
  out->ra = in->ra + (corr / 3600) * sin (ln_deg_to_rad (in->ra));
  out->dec = in->dec;
}

void
applyDCEC (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	   double a1, double a2, double phi)
{
  out->ra = in->ra;
  out->dec = in->dec + (corr / 3600) * cos (ln_deg_to_rad (in->dec));
}

void
applyDCES (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	   double a1, double a2, double phi)
{
  out->ra = in->ra;
  out->dec = in->dec + (corr / 3600) * sin (ln_deg_to_rad (in->dec));
}

// Aux1 to h (for Paramount, where it's unsure)
// status: testing
void
applyA1H (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2, double phi)
{
  out->ra = in->ra - ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * a1);
//    ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * ln_deg_to_rad (a1));
  out->dec = in->dec;
}

// Aux1 to in \delta (for Paramount, where it's unsure)
// status: testing
void
applyA1D (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	  double a1, double a2, double phi)
{
  out->dec = in->dec - ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * a1);
//    ln_rad_to_deg (ln_deg_to_rad (corr / 3600) * ln_deg_to_rad (a1));
  out->ra = in->ra;
}

void
applyTF (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  double d, h, f;
  d = ln_deg_to_rad (in->dec);
  h = ln_deg_to_rad (in->ra);
  f = ln_deg_to_rad (phi);

  out->ra = in->ra + (corr / 3600) * cos (f) * sin (h) / cos (d);
  out->dec =
    in->dec +
    (corr / 3600) * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d));
}

void
applyTX (struct ln_equ_posn *in, struct ln_equ_posn *out, double corr,
	 double a1, double a2, double phi)
{
  double d, h, f;
  d = ln_deg_to_rad (in->dec);
  h = ln_deg_to_rad (in->ra);
  f = ln_deg_to_rad (phi);

  out->ra =
    in->ra +
    (corr / 3600) * cos (f) * sin (h) / cos (d) /
    (cos (d) * sin (f) + cos (d) * cos (h) * cos (f));
  out->dec =
    in->dec +
    (corr / 3600) * (cos (f) * cos (h) * sin (d) - sin (f) * cos (d)) /
    (cos (d) * sin (f) + cos (d) * cos (h) * cos (f));
}

//    h -= TP_A1H / 3600 * (double) flip;
//    d -= TP_A1D / 3600 * (double) flip;

void
applyProperMotion (struct ln_equ_posn *in, struct ln_equ_posn *out,
		   struct ln_equ_posn *pm, double JD)
{
  struct ln_equ_posn result;

  ln_get_equ_pm (in, pm, JD, &result);

  out->ra = result.ra;
  out->dec = result.dec;
}

void
applyAberation (struct ln_equ_posn *in, struct ln_equ_posn *out, double JD)
{
  struct ln_equ_posn result;

  ln_get_equ_aber (in, JD, &result);

  out->ra = result.ra;
  out->dec = result.dec;
}


void
applyPrecession (struct ln_equ_posn *in, struct ln_equ_posn *out, double JD)
{
  struct ln_equ_posn result;

  ln_get_equ_prec (in, JD, &result);

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

  ln_get_hrz_from_equ (in, obs, JD, &hrz);

  ref = get_refraction (hrz.alt);	//, 1010.0, 10.0);

  hrz.alt += ref;
  ln_get_equ_from_hrz (&hrz, obs, JD, &result);

  result.ra = in360 (result.ra);
  result.dec = in180 (result.dec);

  out->ra = result.ra;
  out->dec = result.dec;
}

struct tpa_termref
{
  void (*apply) (struct ln_equ_posn * in, struct ln_equ_posn * out,
		 double corr, double a1, double a2, double phi);
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
  applyFO, "FO"},
  {
  applyCH, "CH"},
  {
  applyNP, "NP"},
  {
  applyPHH, "PHH"},
  {
  applyPDD, "PDD"},
  {
  applyA1H, "A1H"},
  {
  applyA1D, "A1D"},
  {
  applyTX, "TX"},
  {
  applyTF, "TF"},
  {
  applyHCEC, "HCEC"},
  {
  applyHCES, "HCES"},
  {
  applyDCEC, "DCEC"},
  {
  applyDCES, "DCES"}
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
      if (!model->term[i]->apply)
	printf ("unimplemented model %s\n", model->term[i]->name);
    }

  fclose (mf);

  return 0;
}

void
release_model ()
{
  int i;

  for (i = 0; i < model->terms; i++)
    free (model->term[i]);
}

#ifdef DEBUG
void
bonz (struct ln_equ_posn *pos)
{
  fprintf (stderr, "\t[%f,%f]\n", pos->ra, pos->dec);
}
#else
#define bonz(a)
#endif

int
tpoint_correction (struct ln_equ_posn *mean_pos,	/* mean pos of the object to go */
		   struct ln_equ_posn *proper_motion,	/* proper motion of the object: may be NULL */
		   struct ln_lnlat_posn *obs,	/* observer location. if NULL: no refraction */
		   double JD,	/* time */
		   double aux1, double aux2,	/* auxiliary reading */
		   struct ln_equ_posn *tel_pos,	/* return: coords corrected for the model */
		   int back	/* reverse the correction? */
  )
{
  int i, ret = 0;
  double Q;
  double _r, _d;

  Q = ln_get_apparent_sidereal_time (JD) * 360.0 / 24.0;

  if (aux1 > 0.5)
    get_model (MODEL_PATH_F);
  else
    get_model (MODEL_PATH_N);


  tel_pos->ra = mean_pos->ra;
  tel_pos->dec = mean_pos->dec;

//  bonz (tel_pos);

  _r = tel_pos->ra;
  _d = tel_pos->dec;

  if (proper_motion)		/* pm may be omitted */
    applyProperMotion (tel_pos, tel_pos, proper_motion, JD);
  applyAberation (tel_pos, tel_pos, JD);
  applyPrecession (tel_pos, tel_pos, JD);

  {
    char z = '+';
    double d, dm, ds, r, rm, rs;
    d = tel_pos->dec;
    r = tel_pos->ra;
    if (d < 0)
      {
	z = '-';
	d = -d;
      }
    dm = floor (60 * (d - floor (d)));
    ds = 60 * (60 * d - floor (60 * d));
    d = floor (d);

    r /= 15;
    rm = floor (60 * (r - floor (r)));
    rs = 60 * (60 * r - floor (60 * r));
    r = floor (r);


    printf
      ("\033[3;3HCURRENT COORD: [%.4f:%.4f] %02.0f %02.0f %05.2f %c%02.0f %02.0f %04.1f   \n",
       tel_pos->ra, tel_pos->dec, r, rm, rs, z, d, dm, ds);
  }

  if (obs)
    {
      applyRefraction (tel_pos, tel_pos, obs, JD);
    }

  if (back)
    {
      tel_pos->ra = 2 * _r - tel_pos->ra;
      tel_pos->dec = 2 * _d - tel_pos->dec;
    }

//     bonz (tel_pos);

  tel_pos->ra = in180 (Q - tel_pos->ra);	// make ha

//     bonz (tel_pos);

  for (i = 0; i < model->terms; i++)
    if (model->term[i]->apply)
      {
//#ifdef DEBUG
//      fprintf(stderr,"%s", model->term[i]->name);
//#endif

	_r = tel_pos->ra;
	_d = tel_pos->dec;

	model->term[i]->apply (tel_pos, tel_pos, model->term[i]->value, aux1,
			       aux2, 37.1);

	// reverse direction
	if (!back)
	  {
	    tel_pos->ra = 2 * _r - tel_pos->ra;
	    tel_pos->dec = 2 * _d - tel_pos->dec;
	  }

//      bonz (tel_pos);
      }

  tel_pos->ra = in360 (Q - tel_pos->ra);	// make ra from ha

//  bonz (tel_pos);
//
  release_model ();

  return ret;
}

static struct ln_lnlat_posn lazy_observer = {
  -6.732778,
  37.104444
//  -14.783639,
//  +49.910556
};

/* lazy func: correct me there coords simply (no proper motion, now!, in place) */
// everything in degrees */
void
tpoint_apply_now (double *ra, double *dec, double aux1, double aux2, int rev)
{
  struct ln_equ_posn equ;
  struct ln_equ_posn mean_pos;
  double JD;
  int ret = 0;

//printf("[%d]", rev);

  mean_pos.ra = *ra;
  mean_pos.dec = *dec;

  JD = ln_get_julian_from_sys ();

  ret =
    tpoint_correction (&mean_pos, NULL, &lazy_observer, JD, aux1, aux2, &equ,
		       rev);

  // If error happened do not modify values
  if (ret)
    return;

  *ra = equ.ra;
  *dec = equ.dec;
}

#ifdef TUNE
double
sid_time ()
{
  return in360 (15 *
		ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
		lazy_observer.lng);
}

void
f2dms (float f, char *ret, char sign)
{
  int d, m;
  float s, q;

  if (f < 0)
    {
      sign = '-';
      f = -f;
    }

  d = (int) f;

  m = (int) (60.0 * (f - (float) d));

  q = (float) d + (float) m / 60.0;

  s = 3600 * (f - q);

  if (s >= 59.5)
    {
      s = 0;
      m += 1;
    }

  if (m == 60)
    {
      m = 0;
      d += 1;
    }

  printf (" %c%02d %02d %04.1f", sign, d, m, s);
}

int
main ()
{
  int i, j, flip, shour;
  double JD, sidtime;
  double r, d = 0, h;

  time_t cas;
  struct tm kk;

  /* Tpoint data header */
  time (&cas);
  gmtime_r (&cas, &kk);
  JD = ln_get_julian_from_sys ();


  printf ("BOOTES: test data\n");
  printf (":EQUAT\n");
  printf (":NODA\n");

  f2dms (lazy_observer.lat, NULL, '+');
  printf (" %d %d %d 15 1013.6\n", kk.tm_year + 1900, kk.tm_mon + 1,
	  kk.tm_mday);

  /* some observations  */

  for (j = -5; j < 10; j++)
    {
      for (i = 0; i < 15; i++)
	{

	  r = i * 24.0;
	  d = j * 8.0;

	  h = in180 (sid_time () - r);

	  fprintf (stderr, "h=%f\n", h);

	  if (h < 0)
	    flip = 1;
	  else
	    flip = 0;

	  f2dms (r / 15, NULL, ' ');
	  f2dms (d, NULL, '+');

	  printf (" 0 0 2000 ", r, d);

	  //ra=r;de=d

	  tpoint_apply_now (&r, &d, (float) flip, 0, 0);

	  // reverse the direction of the correction :*)
	  //r=2*ra-r;
	  //d=2*de-d;

	  f2dms (r / 15, NULL, ' ');
	  f2dms (d, NULL, '+');

	  sidtime = sid_time () / 15;
	  shour = (int) sidtime;
	  printf ("   %02d %05.2f %d\n", shour,
		  (sidtime - (float) shour) * 60.0, flip);
	}
    }

  printf ("END");

}

#endif
