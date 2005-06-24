#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "tpmodel.h"
#include "libmks3.h"

#include "telescope.h"

#define LONGITUDE -6.732778
#define LATITUDE 37.104444
#define ALTITUDE 10.0
#define LOOP_DELAY 200000

/* RAW model: Tpoint models should be computed as addition to this one */
#define DEC_CPD -20880.0	// DOUBLE - how many counts per degree...
#define HA_CPD -32000.0		// 8.9 counts per arcsecond
// Podle dvou ruznych modelu je to bud <-31845;-31888>, nebo <-31876;-31928>, => -31882
// pripadne <32112;32156> a <32072;32124> => -32118 (8.9216c/") podle toho, jestli se
// to ma pricist nebo odecist, coz nevim. -m.

#define DEC_ZERO (-0.380*DEC_CPD)	// DEC homing: -2.079167 deg
#define HA_ZERO (-29.880*HA_CPD)	// AFTERNOON ZERO: -30 deg

typedef enum
{
  TEL_POINT,
  // Sidereal mode, telescope points to an object. Is allowed to give commands, 
  // if it somehow gets to a dangerous position, it will be
  // stopped, if it will need rehoming, will recieve it
  TEL_SLEWING,
  // Slewing to a position: after recieving position change. Cannot give telescope
  // commands apart of killing of the slew (then recycle and
  // slew from "point" state
  TEL_STOP,
  // Motors are off: before doing anything (if requested) is needed to switch them on
  TEL_HOMING,
  // one of tel axes does homing: nothing can we do until it reaches homing
  TEL_TIMEOUT,
  // Connection to telescope has not been reached: no commands until we'll 
  // know what's up
//    TEL_JOYSTICK// Somebody touches joystick at the telescope?
}
TEL_STATE;

class Rts2DevTelescopeParamount:public Rts2DevTelescope
{
private:
  MKS3Id axis0, axis1;
  int ret0, ret1;
  unsigned short stat0, stat1;	// axis status

  struct ln_lnlat_posn observer;

  double sid_home ();

  // *** return siderial time in degrees ***
  // once libnova will use struct timeval in
  // ln_ln_get_julian_from_sys, this will be completely OK
  //
  double sid_time (double julian)
  {
    return ln_range_degrees (15 * ln_get_apparent_sidereal_time (julian) +
			     LONGITUDE);
  }
  double gethoming (double sid);
  double x180 (double x);

  void counts2sky (long ac, long dc, double *ra, double *dec, int *flip);
  int sky2counts (double ra, double dec, long *ac, long *dc);
  TEL_STATE state;
  char *device_file;
protected:
  virtual int stopMove ();
public:
  Rts2DevTelescopeParamount (int argc, char **argv);
  virtual ~ Rts2DevTelescopeParamount (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int idle ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();
};

Rts2DevTelescopeParamount::Rts2DevTelescopeParamount (int argc, char **argv):
Rts2DevTelescope (argc, argv)
{
  telSiderealTime = 0;

  axis0.unitId = 0x64;
  axis1.unitId = 0x64;

  axis0.axisId = 0;
  axis1.axisId = 1;

  device_file = "/dev/ttyS0";
  addOption ('f', "device_file", 1,
	     "/dev/ttySx entry for Paramount connector");

  strcpy (telType, "ParamountME");
  strcpy (telSerialNumber, "-");
}

Rts2DevTelescopeParamount::~Rts2DevTelescopeParamount (void)
{
  // some done?
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

// return sid time since homing
double
Rts2DevTelescopeParamount::sid_home ()
{
  long pos0, en0, sidhome;

  if (MKS3PosEncoderGet (axis0, &en0))
    return 0;
  if (MKS3PosCurGet (axis0, &pos0))
    return 0;

  sidhome = en0 - pos0;
  return (double) sidhome / HA_CPD;
}

double
Rts2DevTelescopeParamount::gethoming (double sid)
{
  static double home, oldhome = 0;

  home = sid - sid_home ();

  if (fabs (home - oldhome) > .001)	// 3.6" 
    oldhome = home;

  return oldhome;
}

/* compute RAW trasformation (no Tpoint) */
void
Rts2DevTelescopeParamount::counts2sky (long ac, long dc, double *ra,
				       double *dec, int *flip)
{
  double _ra, _dec, JD;

  // Base transform to raw RA, DEC    
  JD = ln_get_julian_from_sys ();
  telSiderealTime = sid_time (JD);

  _dec = (double) (dc - DEC_ZERO) / DEC_CPD;
  _ra = (double) (ac - HA_ZERO) / HA_CPD;
  _ra = -_ra + gethoming (telSiderealTime);

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
  _ra = ln_range_degrees (_ra);

  printf ("TP <-: %f %f ->", _ra, _dec);
  tpoint_apply_now (&_ra, &_dec, (double) (*flip), 0.0, 1);
  printf ("%f %f\n", _ra, _dec);

  *dec = _dec;
  *ra = _ra;
}

int
Rts2DevTelescopeParamount::sky2counts (double ra, double dec, long *ac,
				       long *dc)
{
  long _dc, _ac, flip = 0;
  double ha, JD;
  struct ln_equ_posn aber_pos, pos;
  struct ln_hrz_posn hrz;

  JD = ln_get_julian_from_sys ();
  telSiderealTime = sid_time (JD);

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
  ha = x180 (telSiderealTime - ra);
// xxx FLIP xxx
  if (ha < 0)
    flip = 1;

  pos.ra = ra;
  pos.dec = dec;
  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);

  if (hrz.alt < 0)
    return -1;

  printf ("TP ->: %f %f ->", ra, dec);
  tpoint_apply_now (&ra, &dec, (double) flip, 0.0, 0);
  printf ("%f %f\n", ra, dec);

//  pos.ra = ra;
//  pos.dec = dec;
//  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);
//  printf ("<%f, %f,%f,%f>\n", ra, ha, dec, hrz.alt);

  // now finally
  ha = x180 (telSiderealTime - ra);
  ra = x180 (gethoming (telSiderealTime) - ra);

  if (flip)
    {
      dec = 180 - dec;
      ra = 180 + ra;
    }

  _dc = (long) (DEC_ZERO + (DEC_CPD * dec));
  _ac = (long) (HA_ZERO + (HA_CPD * ra));

  *dc = _dc;
  *ac = _ac;

//  if (hrz.alt < 0)
//    return -1;
  return 0;
}

double
Rts2DevTelescopeParamount::x180 (double x)
{
  for (; x > 180; x -= 360);
  for (; x < -180; x += 360);
  return x;
}

int
Rts2DevTelescopeParamount::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescopeParamount::init ()
{
  int ret;
  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  state = TEL_POINT;		// fool-proof beginning...

  while (1)
    {
      ret = MKS3Init (device_file);
      if (!ret)
	break;

      syslog (LOG_DEBUG,
	      "Rts2DevTelescopeParamount::init cannot call MKS3Init, waiting 1 minute");
      sleep (60);
    }

  return ret;
}

int
Rts2DevTelescopeParamount::idle ()
{
  long axis0_counts;
  long axis1_counts;
  // get telescope coordinates
  ret0 = MKS3PosCurGet (axis0, &axis0_counts);
  ret1 = MKS3PosCurGet (axis1, &axis1_counts);
  counts2sky (axis0_counts, axis1_counts, &telRa, &telDec, &telFlip);

  // TELESCOPE PART: zjistit, jak se ma dalekohled (update statutu,
  // tj.  slew/neslew/motor stoji/je potreba uhoumovat/uz se
  // nehoumuje)

  ret0 = MKS3StatusGet (axis0, &stat0);
  ret1 = MKS3StatusGet (axis1, &stat1);

  if (sky2counts (telRa, telDec, &axis0_counts, &axis1_counts))	// if failed, park us..
    {
      startPark ();
      maskState (0, TEL_MASK_MOVING, TEL_PARKING, "forced telescope park");
    }

  if (ret0 || ret1)
    {
      if (state != TEL_TIMEOUT)
	{
	  syslog (LOG_DEBUG,
		  "Didn't get status..h:%d d:%d (setting state to TEL_TIMEOUT)",
		  ret0, ret1);
	  state = TEL_TIMEOUT;
	}
      return Rts2DevTelescope::idle ();
    }
  else
    {
      if (state == TEL_TIMEOUT)
	{
	  // we don't like to be distributed for next 10 seconds..
	  syslog (LOG_DEBUG, "Telescope's back! (wait 10s)");
	  state = TEL_POINT;
	  sleep (10);
	}
    }
  // handle homing..
  if (!(stat0 & MOTOR_HOMED) && !(stat0 & MOTOR_HOMING))
    {
      MKS3Home (axis0, 0);
      state = TEL_HOMING;
      maskState (0, TEL_MASK_MOVING, TEL_PARKING, "forced homing (ra axis)");
    }
  if (!(stat0 & MOTOR_HOMED) && !(stat0 & MOTOR_HOMING))
    {
      MKS3Home (axis0, 0);
      state = TEL_HOMING;
      maskState (0, TEL_MASK_MOVING, TEL_PARKING, "forced homing (dec axis)");
    }

  if (stat0 & MOTOR_SLEWING || stat1 & MOTOR_SLEWING)
    {
      if (state != TEL_SLEWING)
	{
	  maskState (0, TEL_MASK_MOVING, TEL_MOVING,
		     "moving started (paramount)");
	  state = TEL_SLEWING;
	}
    }

  return Rts2DevTelescope::idle ();
}

int
Rts2DevTelescopeParamount::ready ()
{
  return state != TEL_TIMEOUT;
}

int
Rts2DevTelescopeParamount::baseInfo ()
{
  return state != TEL_TIMEOUT;
}

int
Rts2DevTelescopeParamount::info ()
{
  return state != TEL_TIMEOUT;
}

int
Rts2DevTelescopeParamount::startMove (double tar_ra, double tar_dec)
{
  long ac, dc;
  if (!sky2counts (tar_ra, tar_dec, &ac, &dc))
    {
      state = TEL_SLEWING;
      ret0 = MKS3PosTargetSet (axis0, ac);
      ret1 = MKS3PosTargetSet (axis1, dc);
      if (ret0 == MAIN_AT_LIMIT)
	{
	  // home if we reached limit (= stop motors)
	  MKS3Home (axis0, 0);
	}
      if (ret1 == MAIN_AT_LIMIT)
	{
	  MKS3Home (axis1, 1);
	}
      return (ret0 || ret1 ? -1 : 0);	// some other error..
    }
  return -1;
}

int
Rts2DevTelescopeParamount::isMoving ()
{
  if (stat0 & MOTOR_OFF || stat1 & MOTOR_OFF)
    {
      state = TEL_STOP;
      return -2;
    }
  return LOOP_DELAY;
}

int
Rts2DevTelescopeParamount::endMove ()
{
  ret0 = MKS3MotorOn (axis0);
  ret1 = MKS3MotorOn (axis1);
  state = TEL_POINT;
  return (ret0 || ret1 ? -1 : 0);
}

int
Rts2DevTelescopeParamount::stopMove ()
{
  ret0 = MKS3PosAbort (axis0);
  ret1 = MKS3PosAbort (axis1);
  usleep (LOOP_DELAY);
  return (ret0 || ret1 ? -1 : 0);
}

int
Rts2DevTelescopeParamount::startPark ()
{
  ret0 = MKS3MotorOff (axis0);
  ret1 = MKS3MotorOff (axis1);
  return (ret0 || ret1 ? -1 : 0);
}

int
Rts2DevTelescopeParamount::isParking ()
{
  if (stat0 & MOTOR_HOMING || stat1 && MOTOR_HOMING)
    {
      // check again for homing..
      return LOOP_DELAY;
    }
  if (stat0 & MOTOR_OFF || stat1 & MOTOR_OFF)
    {
      return -2;
      state = TEL_STOP;
    }
  MKS3MotorOff (axis0);
  MKS3MotorOff (axis1);
  return LOOP_DELAY;
}

int
Rts2DevTelescopeParamount::endPark ()
{
  return 0;
}

Rts2DevTelescopeParamount *device;

void
switchoffSignal (int sig)
{
  syslog (LOG_DEBUG, "switchoffSignal signal: %i", sig);
  delete device;
  exit (1);
}


int
main (int argc, char **argv)
{
  device = new Rts2DevTelescopeParamount (argc, argv);

  signal (SIGINT, switchoffSignal);
  signal (SIGTERM, switchoffSignal);

  int ret = -1;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot find telescope, exiting\n");
      exit (1);
    }
  device->run ();
  delete device;
}
