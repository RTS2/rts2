#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include "libmks3.h"
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "maps.h"
#include "tpmodel.h"

#define LONGITUDE -6.732778
#define LATITUDE 37.104444
#define ALTITUDE 10.0
#define LOOP_DELAY 200000

#define DEF_SLEWRATE 1080000000.0
//#define RA_SLEW_SLOWER 0.75
#define RA_SLEW_SLOWER 1.00
//#define DEC_SLEW_SLOWER 0.90
#define DEC_SLEW_SLOWER 1.00

#define PARK_DEC -0.380
#define PARK_HA -29.880

/* RAW model: Tpoint models should be computed as addition to this one */
#define DEC_CPD -20880.0	// DOUBLE - how many counts per degree...
#define HA_CPD -32000.0		// 8.9 counts per arcsecond
// Podle dvou ruznych modelu je to bud <-31845;-31888>, nebo <-31876;-31928>, => -31882
// pripadne <32112;32156> a <32072;32124> => -32118 (8.9216c/") podle toho, jestli se
// to ma pricist nebo odecist, coz nevim. -m.

#define DEC_ZERO (-0.380*DEC_CPD)	// DEC homing: -2.079167 deg
#define HA_ZERO (-29.880*HA_CPD)	// AFTERNOON ZERO: -30 deg

double lasts = 0;

typedef struct
{
  MKS3Id axis0, axis1;
  double lastra, lastdec;
}
T9;

char statestr[8][32] = { "POINT", "SLEWING", "HOMING", "TIMEOUT", "STOP" };

double
x180 (double x)
{
  for (; x > 180; x -= 360);
  for (; x < -180; x += 360);
  return x;
}

double
x360 (double x)
{
  for (; x > 360; x -= 360);
  for (; x < 0; x += 360);
  return x;
}

void
display_status (long status, int color)
{
  if (status & MOTOR_HOMING)
    fprintf (stdout, "\033[%dmHOG\033[m ", 30 + color);
//  if (status & MOTOR_SERVO)
//    printf ("\033[%dmSRV\033[m ", 30 + color);
  if (status & MOTOR_INDEXING)
    fprintf (stdout, "\033[%dmIDX\033[m ", 30 + color);
  if (status & MOTOR_SLEWING)
    fprintf (stdout, "\033[%dmSLW\033[m ", 30 + color);
  if (!(status & MOTOR_HOMED))
    fprintf (stdout, "\033[%dm!HO\033[m ", 30 + color);
  if (status & MOTOR_JOYSTICKING)
    fprintf (stdout, "\033[%dmJOY\033[m ", 30 + color);
  if (status & MOTOR_OFF)
    fprintf (stdout, "\033[%dmOFF\033[m ", 30 + color);
}



// GLOBAL DECLARATIONS
// It's pretty convenient to declare this globally
struct ln_lnlat_posn observer;
static T9 *mount = NULL;
//char anim[]=".oOo";
char anim[] = "|/-\\";

// return sid time since homing
double
sid_home ()
{
  long pos0, en0, sidhome;

  if (MKS3PosEncoderGet (mount->axis0, &en0))
    return 0;
  if (MKS3PosCurGet (mount->axis0, &pos0))
    return 0;

//  printf ("[sid_home=%.4f]\n", (double) (en0 - pos0) / HA_CPD);
//  fflush (stdout);

  sidhome = en0 - pos0;


  return (double) sidhome / HA_CPD;
}

// *** return siderial time in degrees ***
// once libnova will use struct timeval in
// ln_ln_get_julian_from_sys, this will be completely OK
//

#define sid_time(julian) x360(15 * ln_get_apparent_sidereal_time (julian) + LONGITUDE)

/*
double
sid_time ()
{
  double s = x360 (15 *
		ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
		LONGITUDE);

//  printf ("[sid_time=%.4f]\n", s);
//  fflush (stdout);

  return s;
}*/

double
gethoming (double sid)
{
  static double home, oldhome = 0;

  home = sid - sid_home ();

  if (fabs (home - oldhome) > .001)	// 3.6" 
    oldhome = home;

//  printf ("[h=%f,o=%f]", home, oldhome);
//  fflush (stdout);

  return oldhome;
}

/* compute RAW trasformation (no Tpoint) */
void
counts2sky (long ac, long dc, double *ra, double *dec, int *flip)
{
  double _ra, _dec, JD, s;

  // Base transform to raw RA, DEC    
  JD = ln_get_julian_from_sys ();
  s = sid_time (JD);
  lasts = s;			// buffer for display only

  _dec = (double) (dc - DEC_ZERO) / DEC_CPD;
  _ra = (double) (ac - HA_ZERO) / HA_CPD;
  _ra = -_ra + gethoming (s);


  // Flip
  if (_dec > 90)
    {
      *flip = 1;
      _dec = 180 - _dec;
      _ra = 180 + _ra;
    }
  else
    *flip = 0;

  _dec = x180 (_dec);
  _ra = x360 (_ra);

  printf ("TP <-: %f %f ->", _ra, _dec);
  tpoint_apply_now (&_ra, &_dec, (double) (*flip), 0.0, 1);
  printf ("%f %f\n", _ra, _dec);

  *dec = _dec;
  *ra = _ra;
}

int
sky2counts (double ra, double dec, long *ac, long *dc)
{
  long _dc, _ac, flip = 0;
  double ha, JD, s;
  struct ln_equ_posn aber_pos, pos;
  struct ln_hrz_posn hrz;

  JD = ln_get_julian_from_sys ();
  s = sid_time (JD);

// Aberation
//  ln_get_equ_aber (&pos, JD, &aber_pos);
// Precession
//  ln_get_equ_prec (&aber_pos, JD, &pos);
// Refraction 
  //  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);
//  hrz.alt += get_refraction (hrz.alt);
//  ln_get_equ_from_hrz (&hrz, &observer, JD, &pos);
//  ra = pos.ra;
//  dec = pos.dec;

// True Hour angle
// first to decide the flip...
  ha = x180 (s - ra);
// xxx FLIP xxx
  if (ha < 0)
    flip = 1;

  pos.ra = ra;
  pos.dec = dec;
  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);

  if (hrz.alt < 0)
    return -1;

//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);

  printf ("TP ->: %f %f ->", ra, dec);
  tpoint_apply_now (&ra, &dec, (double) flip, 0.0, 0);
  printf ("%f %f\n", ra, dec);

//  pos.ra = ra;
//  pos.dec = dec;
//  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);
//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);

  // now finally
  ha = x180 (s - ra);
//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);
  ra = x180 (gethoming (s) - ra);

//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);

  if (flip)
    {
      dec = 180 - dec;
      ra = 180 + ra;
    }

//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);

  _dc = DEC_ZERO + (long) (DEC_CPD * dec);
  _ac = HA_ZERO + (long) (HA_CPD * ra);

//  printf ("<%ld, %ld>\n", _ac, _dc);

  *dc = _dc;
  *ac = _ac;

//  if (hrz.alt < 0)
//    return -1;
  return 0;
}


main ()
{
  TEL_STATE state = TEL_POINT;	// fool-proof beginning...
  char device[256] = "/dev/ttyS0";
  int ret0 = 0, ret1 = 0, ghi = 0;
  unsigned short stat0, stat1;
  double ra = 0, dec = 0;
  int park = 1, update = 0;
  long ac, dc, oac = 0, odc = 0;
  FILE *file;

  // Read config
  // XXX do that !!!: this is not enough
  observer.lat = LATITUDE;
  observer.lng = LONGITUDE;
  // tpmodel_file=

  mk_maps ();

  Tstat->serial_number[63] = state;
  sprintf (Tstat->type, "ParamountME");
  sprintf (Tstat->serial_number, "-");
  Tstat->park_dec = 90;
  Tstat->longtitude = LONGITUDE;
  Tstat->latitude = LATITUDE;
  Tstat->altitude = ALTITUDE;

  mount = (T9 *) malloc (sizeof (T9));

  mount->axis0.unitId = 0x64;
  mount->axis1.unitId = 0x64;

  mount->axis0.axisId = 0;
  mount->axis1.axisId = 1;

// A co kdyz je teleskop vypnuty! - MKS3Init vraci MKS_OK, jinak kod chyby...

  fprintf (stdout, "init: %s\n", device);

  ret0 = MKS3Init (device);
  if (ret0)
    {
      fprintf (stdout, "%s: MKS3Init returned=%d\n", __FUNCTION__, ret0);
      exit (ret0);
    }

  while (1)			// furt 
    {
      update = 0;
      // otevrit RO fajl s pozadavkem porovnat target_ra a target_dec s
      // fajlem kdyz se lisi, premastit ty co mam u sebe a nastavit flag
      // aktualizace, pokud je tam pozadavek na parkovani, nastavit podle
      // toho pozadavek na parkovani interne.
      {
	int npark;
	struct ln_equ_posn npos;
	struct ln_hrz_posn hrz;
	double JD, nra, ndec;
	int len, num;
	char buf[1024];

#if 0
	file = fopen ("telfile", "r");
	rewind (file);
	len = fread (buf, 1, 1024, file);

	num = fclose (file);
	if (num)
	  fprintf (stdout, "fclose(file)=%d\n", num);

	if (len > 0)
#endif
	  {
#if 1
	    // First possibility
	    {
	      npos.ra = x360 (Tctrl->ra);
	      npos.dec = x180 (Tctrl->dec);
	      npark = Tctrl->power;
	    }

#else
	    // The other possibility
	    {
	      int file;
	      file = open ("telfile", O_RDONLY);
	      len = read (file, buf, 1024);
	      close (file);

	      num = sscanf (buf, "%lf\n%lf\n%d", &nra, &ndec, &npark);
	      npos.ra = x360 (nra);
	      npos.dec = x180 (ndec);
	    }
#endif

	    //printf("(%d,%d) %f %f %d\n",len,num,nra,ndec,npark);

	    JD = ln_get_julian_from_sys ();
	    ln_get_hrz_from_equ (&npos, &observer, JD, &hrz);

	    if (hrz.alt < 0)
	      npark = 1;

//          printf ("{%.1f %+.1f %+.1f %d} ", npos.ra, npos.dec, hrz.alt, npark);
//              fflush (stdout);

	    if ((npos.ra != ra) || (npos.dec != dec) || (npark != park))
	      {
//              go = 1;
		ra = npos.ra;
		dec = npos.dec;
		park = npark;

		update = 1;
//              printf ("{%.1f %.1f %d}", ra, dec, park);
//              fflush (stdout);
	      }
	  }
      }

      // TELESCOPE PART: zjistit, jak se ma dalekohled (update statutu,
      // tj.  slew/neslew/motor stoji/je potreba uhoumovat/uz se
      // nehoumuje)
      //
      ret0 = MKS3StatusGet (mount->axis0, &stat0);
      ret1 = MKS3StatusGet (mount->axis1, &stat1);

      if (ret0 || ret1)
	{
	  if (state != TEL_TIMEOUT)
	    {
	      fprintf
		(stdout,
		 "\033[1mDidn't get status... h:%d d:%d (setting state to TEL_TIMEOUT)\033[m\n",
		 ret0, ret1);
	      fflush (stdout);
	    }
	  state = TEL_TIMEOUT;
	}
      else
	{
	  if (state == TEL_TIMEOUT)
	    {
	      fprintf (stdout, "\033[1mTelescope's back!(wait 10s)\033[m\n",
		       ret0, ret1);
	      fflush (stdout);
	      sleep (10);	// TIMEOUT OF TIMEOUT :)
	    }
	  state = TEL_POINT;

	  ret0 = MKS3PosCurGet (mount->axis0, &ac);
	  ret1 = MKS3PosCurGet (mount->axis1, &dc);
	  counts2sky (ac, dc, &(Tstat->ra), &(Tstat->dec), &(Tstat->flip));
	  Tstat->axis0_counts = ac;
	  Tstat->axis1_counts = dc;
	  Tstat->siderealtime = lasts;
	  //fprintf(stdout, "%f %.5f %+.5f/%c ", lasts, Tstat->ra, Tstat->dec, Tstat->flip?'f':'n');

//        display_status (stat0, 1);
//        display_status (stat1, 2);

//        printf("[%.2f %.2f %d] --\n",ra,dec,park);
//        fflush (stdout);

	  if (stat0 & MOTOR_HOMING || stat1 & MOTOR_HOMING)
	    state = TEL_HOMING;
	  if (stat0 & MOTOR_SLEWING || stat1 & MOTOR_SLEWING)
	    state = TEL_SLEWING;
	  if (stat0 & MOTOR_OFF || stat1 & MOTOR_OFF)
	    state = TEL_STOP;

//          fprintf(stdout,"\033[1,34m(.,%s)\033[m \n", statestr[state]);fflush(stdout);

	  // stat[01] is valid, let's check for homings
	  if (state == TEL_POINT
	      && (!(stat0 & MOTOR_HOMED)) /* && (!(stat0 & MOTOR_HOMING)) */ )
	    {
	      ret0 = MKS3Home (mount->axis0, 0);
	      fprintf
		(stdout,
		 "\033[1mHome:... h:%d (setting state to TEL_HOMING)\033[m\n",
		 ret0, ret1);
	      //fflush (stdout);

	      state = TEL_HOMING;
	    }
	  if ((state == TEL_POINT)
	      && (!(stat1 & MOTOR_HOMED)) /* && (!(stat1 & MOTOR_HOMING)) */ )
	    {
	      ret1 = MKS3Home (mount->axis1, 0);
	      fprintf
		(stdout,
		 "\033[1mHome:... d:%d (setting state to TEL_HOMING)\033[m\n",
		 ret1);
	      //fflush (stdout);

	      state = TEL_HOMING;
	    }
	}

      if ((state == TEL_SLEWING) && (update))
	{
	  ret0 = MKS3PosAbort (mount->axis0);
	  ret1 = MKS3PosAbort (mount->axis1);

	  usleep (LOOP_DELAY);
	}

      if ((state == TEL_STOP) && (!park))
	{
	  ret0 = MKS3MotorOn (mount->axis0);
	  ret1 = MKS3MotorOn (mount->axis1);
	  state = TEL_POINT;
	}

      if ((state == TEL_POINT) && (!park))
	{
	  if (!sky2counts (ra, dec, &ac, &dc))	// it must not fail...
	    {

//              Tstat->ra=ra;
//            Tstat->dec=dec;

//            Tstat->axis0_counts=ac;
//            Tstat->axis1_counts=dc;

	      fprintf (stdout, "%.3f%+.3f/%d ", ra, dec, park /*, ac, dc */ );
//            fflush (stdout);

	      /*if(oac!=ac) */ ret0 = MKS3PosTargetSet (mount->axis0, oac =
							ac);
	      /*if(odc!=dc) */ ret1 = MKS3PosTargetSet (mount->axis1, odc =
							dc);

	      if (ret0)		//==2007) // at-limit
		{
		  fprintf (stdout,
			   "\033[1mMKS3PosTargetSet(h,%ld)=%d\033[m\n", ac,
			   ret0);

		  if (ret0 == MAIN_AT_LIMIT)
		    {
		      ret0 = MKS3Home (mount->axis0, 0);
		      state = TEL_HOMING;
		    }
		}
	      if (ret1)		//==2007) // at-limit
		{
		  fprintf (stdout,
			   "\033[1mMKS3PosTargetSet(d,%ld)=%d\033[m\n", dc,
			   ret1);

		  if (ret0 == MAIN_AT_LIMIT)
		    {
		      ret1 = MKS3Home (mount->axis1, 0);
		      state = TEL_HOMING;
		    }
		}
	      //fflush (stdout);
	    }
	  else
	    park = 1;
	}

      if ((state == TEL_POINT) && park)
	{
	  ret0 = MKS3MotorOff (mount->axis0);
	  ret1 = MKS3MotorOff (mount->axis1);
	  state = TEL_STOP;
	}

      Tstat->serial_number[63] = state;

      // if (stat0 & MOTOR_OFF) (stat1 & MOTOR_SLEWING) !(stat0 &
      // MOTOR_HOMED) (status & MOTOR_HOMING) (status & MOTOR_JOYSTICKING)


      // VSECHNY ZAPISY JSOU ZAKAZANY, POKUD NENI STATUS PARK/POINT
      // ve stavech SLEW/HOMING/TIMEOUT se dela jen stav.
      // ----
      // zjistit, jestli se nezmenil homing point, pripadne ho zjistit
      // (coz asi znamena zjistit a pripadne updatnout, lisi-li se
      // zasadne)
      // ----
      // zjistit pripadne povinnosti plynouci ze zmeny stavu
      // * pokud je treba houmovat: pustit houmink (ve dvou pripadech:
      //      jednak muze chtit home sama montaz,
      //      jednak to muze bejt vysledek NACK pri move)
      // ----
      // jestli slewujeme a mame update, kill slew
      // jestli jsme zaparkovani a mame update: probudit
      // ----
      // jesli muzeme slew a jestli nejsme na countech, kde mame bejt,
      // dalekohled tam poslat, 
      // * jestli vrati NACK, nastavit pozadavek na homing, 
      // * jestli je to pod obzorem, zaparkovat dalekohled a do WO
      // fajlu to nejak dat vedet?..
      // ----
      // COMMUNICATION PART:
      // ----
      // pokud se status zmenil (a asi jo..., leda ze by bylo zaparkovano)
      //
      // nicmene bude asi vhodne schovat puvodni string, ktery jsem
      // narval do souboru a cpat ho tam, jen je-li jiny.
      // 
      // otevrit WO fajl se statutem schovat: state, ra, dec, rawra,
      // rawdec, counts0, counts1, a, z pokud mozno single-write
      // ----


      // ----
      // znovu nacist konfig a model, pokud jsme prijali signal (mela by
      // na to ukazovat nejaka globalni promenna
      //
      fprintf (stdout, "(%c,%s)              \r", anim[ghi], statestr[state]);
      fflush (stdout);

//      snprintf(Tstat->state,32,"%s",statestr[state]);

      ghi++;
      if (ghi > 3)
	ghi = 0;
      usleep (LOOP_DELAY);	// wait a while... so we do not block everything...
    }
}
