#include "telescope.h"
#include "model/telmodel.h"

#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include <libmks3.h>
#include <iostream>
#include <fstream>
#include <vector>

#define TEL_SLEW		0x00
#define TEL_FORCED_HOMING0	0x01
#define TEL_FORCED_HOMING1	0x02

#define RA_TICKS		11520000
#define DEC_TICKS		7500000

// park positions
#define PARK_AXIS0		0
#define PARK_AXIS1		0

typedef enum
{ T16, T32 } para_t;

/**
 * Holds paramount constant values
 */
class ParaVal
{
private:
  const char *name;
  para_t type;
  int id;
public:
    ParaVal (const char *in_name, para_t in_type, int in_id)
  {
    name = in_name;
    type = in_type;
    id = in_id;
  }

  bool isName (std::string in_name)
  {
    return (name == in_name);
  }

  int writeAxis (std::ostream & _os, const MKS3Id & axis);
  int readAxis (std::istream & _is, const MKS3Id & axis);
};

int
ParaVal::writeAxis (std::ostream & _os, const MKS3Id & axis)
{
  CWORD16 val16;
  CWORD32 val32;
  int ret;
  _os << name << ": ";
  switch (type)
    {
    case T16:
      ret = _MKS3DoGetVal16 (axis, id, &val16);
      if (ret)
	{
	  _os << "error" << std::endl;
	  return -1;
	}
      _os << val16 << std::endl;
      break;
    case T32:
      ret = _MKS3DoGetVal32 (axis, id, &val32);
      if (ret)
	{
	  _os << "error" << std::endl;
	  return -1;
	}
      _os << val32 << std::endl;
      break;
    }
  return ret;
}

int
ParaVal::readAxis (std::istream & _is, const MKS3Id & axis)
{
  CWORD16 val16;
  CWORD32 val32;
  int ret;
  switch (type)
    {
    case T16:
      _is >> val16;
      if (_is.fail ())
	return -1;
      ret = _MKS3DoSetVal16 (axis, id, val16);
      break;
    case T32:
      _is >> val32;
      if (_is.fail ())
	return -1;
      ret = _MKS3DoSetVal32 (axis, id, val32);
      break;
    }
  return ret;
}

class Rts2DevTelParamount:public Rts2DevTelescope
{
private:
  MKS3Id axis0;
  MKS3Id axis1;
  char *device_name;
  char *paramount_cfg;

  int ret0;
  int ret1;

  CWORD16 status0;
  CWORD16 status1;

  CWORD32 park_axis[2];

  // that's in degrees
  double haZero;
  double decZero;

  double haCpd;
  double decCpd;

  int checkRetAxis (const MKS3Id & axis, int reta);
  int checkRet ();
  int updateStatus ();

    std::vector < ParaVal > paramountValues;

  int saveAxis (std::ostream & os, const MKS3Id & axis);

  // returns actual home offset
  int getHomeOffset (CWORD32 & off);

  int updateLimits ();

  int sky2counts (double ra, double dec, CWORD32 & ac, CWORD32 & dc);
  int sky2counts (struct ln_equ_posn *pos, CWORD32 & ac, CWORD32 & dc,
		  double JD, CWORD32 homeOff);
  int counts2sky (CWORD32 & ac, CWORD32 dc, double &ra, double &dec);

  int moveState;

  CWORD32 acMin;
  CWORD32 acMax;
  CWORD32 dcMin;
  CWORD32 dcMax;

  int acMargin;

  // variables for tracking
  struct timeval track_start_time;
  struct timeval track_recalculate;
  struct timeval track_next;
  MKS3ObjTrackInfo *track0;
  MKS3ObjTrackInfo *track1;
protected:
    virtual int processOption (int in_opt);
  virtual int isMoving ();
  virtual int isParking ();

  virtual void updateTrack ();
public:
    Rts2DevTelParamount (int in_argc, char **in_argv);
    virtual ~ Rts2DevTelParamount (void);

  virtual int init ();
  virtual int idle ();

  virtual int baseInfo ();
  virtual int info ();

  virtual int startMove (double tar_ra, double tar_dec);
  virtual int endMove ();
  virtual int stopMove ();

  virtual int startPark ();
  virtual int endPark ();

  virtual int correct (double cor_ra, double cor_dec, double real_ra,
		       double real_dec);

  // save and load gemini informations
  virtual int saveModel ();
  virtual int loadModel ();
};

int
Rts2DevTelParamount::checkRetAxis (const MKS3Id & axis, int reta)
{
  char *msg = NULL;
  messageType_t msg_type = MESSAGE_ERROR;
  switch (reta)
    {
    case COMM_OKPACKET:
      msg = "comm packet OK";
      msg_type = MESSAGE_DEBUG;
      break;
    case COMM_NOPACKET:
      msg = "comm no packet";
      break;
    case COMM_TIMEOUT:
      msg = "comm timeout (check cable!)";
      break;
    case COMM_COMMERROR:
      msg = "comm error";
      break;
    case COMM_BADCHAR:
      msg = "bad char at comm";
      break;
    case COMM_OVERRUN:
      msg = "comm overrun";
      break;
    case COMM_BADCHECKSUM:
      msg = "comm bad checksum";
      break;
    case COMM_BADLEN:
      msg = "comm bad message len";
      break;
    case COMM_BADCOMMAND:
      msg = "comm bad command";
      break;
    case COMM_INITFAIL:
      msg = "comm init fail";
      break;
    case COMM_NACK:
      msg = "comm not acknowledged";
      break;
    case COMM_BADID:
      msg = "comm bad id";
      break;
    case COMM_BADSEQ:
      msg = "comm bad sequence";
      break;
    case COMM_BADVALCODE:
      msg = "comm bad value code";
      break;

    case MAIN_WRONG_UNIT:
      msg = "main wrong unit";
      break;
    case MAIN_BADMOTORINIT:
      msg = "main bad motor init";
      break;
    case MAIN_BADMOTORSTATE:
      msg = "main bad motor state";
      break;
    case MAIN_BADSERVOSTATE:
      msg = "main bad servo state";
      break;
    case MAIN_SERVOBUSY:
      msg = "main servo busy";
      break;
    case MAIN_BAD_PEC_LENGTH:
      msg = "main bad pec lenght";
      break;
    case MAIN_AT_LIMIT:
      msg = "main at limit";
      break;
    case MAIN_NOT_HOMED:
      msg = "main not homed";
      break;
    case MAIN_BAD_POINT_ADD:
      msg = "main bad point add";
      break;

    case FLASH_PROGERR:
      msg = "flash program error";
      break;
/*    case FLASH_ERASEERR:
      msg = "flash erase error";
      break; */
    case FLASH_TIMEOUT:
      msg = "flash timeout";
      break;
    case FLASH_CANT_OPEN_FILE:
      msg = "flash cannot open file";
      break;
    case FLASH_BAD_FILE:
      msg = "flash bad file";
      break;
    case FLASH_FILE_READ_ERR:
      msg = "flash file read err";
      break;
    case FLASH_BADVALID:
      msg = "flash bad value id";
      break;

    case MOTOR_OK:
      msg = "motor ok";
      msg_type = MESSAGE_DEBUG;
      break;
    case MOTOR_OVERCURRENT:
      msg = "motor over current";
      break;
    case MOTOR_POSERRORLIM:
      msg = "motor maximum position error exceeded";
      break;
    case MOTOR_STILL_ON:
      msg = "motor still slewing but command needs it stopped";
      break;
    case MOTOR_NOT_ON:
      msg = "motor off";
      break;
    case MOTOR_STILL_MOVING:
      msg = "motor still slewing but command needs it stopped";
      break;
    }
  if (msg && msg_type != MESSAGE_DEBUG)
    logStream (msg_type) << "Axis:" << axis.axisId << " " << msg << sendLog;
  return reta;
}

int
Rts2DevTelParamount::checkRet ()
{
  int reta0, reta1;
  reta0 = checkRetAxis (axis0, ret0);
  reta1 = checkRetAxis (axis1, ret1);
  if (reta0 != MKS_OK || reta1 != MKS_OK)
    return -1;
  return 0;
}

int
Rts2DevTelParamount::updateStatus ()
{
  ret0 = MKS3StatusGet (axis0, &status0);
  ret1 = MKS3StatusGet (axis1, &status1);
  return checkRet ();
}

int
Rts2DevTelParamount::saveAxis (std::ostream & os, const MKS3Id & axis)
{
  int ret;
  CWORD16 pMajor, pMinor, pBuild;

  ret = MKS3VersionGet (axis, &pMajor, &pMinor, &pBuild);
  if (ret)
    return -1;

  os << "*********************** Axis "
    << axis.axisId << (axis.axisId ? " (DEC)" : " (RA)")
    << " Control version " << pMajor << "." << pMinor << "." << pBuild
    << " ********************" << std::endl;

  for (std::vector < ParaVal >::iterator iter = paramountValues.begin ();
       iter != paramountValues.end (); iter++)
    {
      ret = (*iter).writeAxis (os, axis);
      if (ret)
	return -1;
    }
  return 0;
}

int
Rts2DevTelParamount::getHomeOffset (CWORD32 & off)
{
  int ret;
  long en0, pos0;

  ret = MKS3PosEncoderGet (axis0, &en0);
  if (ret != MKS_OK)
    return ret;
  ret = MKS3PosCurGet (axis0, &pos0);
  if (ret != MKS_OK)
    return ret;
  off = en0 - pos0;
  return 0;
}

int
Rts2DevTelParamount::updateLimits ()
{
  int ret;
  ret = MKS3ConstsLimMinGet (axis0, &acMin);
  if (ret)
    return -1;
  ret = MKS3ConstsLimMaxGet (axis0, &acMax);
  if (ret)
    return -1;

  ret = MKS3ConstsLimMinGet (axis1, &dcMin);
  if (ret)
    return -1;
  ret = MKS3ConstsLimMaxGet (axis1, &dcMax);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevTelParamount::sky2counts (double ra, double dec, CWORD32 & ac,
				 CWORD32 & dc)
{
  double JD;
  CWORD32 homeOff;
  struct ln_equ_posn pos;
  int ret;

  JD = ln_get_julian_from_sys ();

  ret = getHomeOffset (homeOff);
  if (ret)
    return -1;

  pos.ra = ra;
  pos.dec = dec;

  return sky2counts (&pos, ac, dc, JD, homeOff);
}

int
Rts2DevTelParamount::sky2counts (struct ln_equ_posn *pos, CWORD32 & ac,
				 CWORD32 & dc, double JD, CWORD32 homeOff)
{
  double lst, ra, dec;
  struct ln_hrz_posn hrz;
  struct ln_equ_posn model_change;
  int ret;
  bool flip = false;

  lst = getLstDeg (JD);

  ln_get_hrz_from_equ (pos, Rts2Config::instance ()->getObserver (), JD,
		       &hrz);
  if (hrz.alt < -1)
    {
      return -1;
    }

  // get hour angle
  ra = ln_range_degrees (lst - pos->ra);
  if (ra > 180.0)
    ra -= 360.0;

  // pretend we are at north hemispehere.. at least for dec
  dec = pos->dec;
  if (telLatitude < 0)
    dec *= -1;

  dec = decZero + dec;
  if (dec > 90)
    dec = 180 - dec;
  if (dec < -90)
    dec = -180 - dec;

  // convert to ac; ra now holds HA
  ac = (CWORD32) ((ra + haZero) * haCpd);
  dc = (CWORD32) (dec * decCpd);

  // gets the limits
  ret = updateLimits ();
  if (ret)
    return -1;

  // purpose of following code is to get from west side of flip
  // on S, we prefer negative values
  if (telLatitude < 0)
    {
      while ((ac - acMargin) < acMin)
	// ticks per revolution - don't have idea where to get that
	{
	  ac += (CWORD32) (RA_TICKS / 2.0);
	  flip = !flip;
	}
    }
  while ((ac + acMargin) > acMax)
    {
      ac -= (CWORD32) (RA_TICKS / 2.0);
      flip = !flip;
    }
  // while on N we would like to see positive values
  if (telLatitude > 0)
    {
      while ((ac - acMargin) < acMin)
	// ticks per revolution - don't have idea where to get that
	{
	  ac += (CWORD32) (RA_TICKS / 2.0);
	  flip = !flip;
	}
    }

  if (flip)
    dc += (CWORD32) ((90 - dec) * 2 * decCpd);

  // put dc to correct numbers
  while (dc < dcMin)
    dc += DEC_TICKS;
  while (dc > dcMax)
    dc -= DEC_TICKS;

  // apply model
  applyModel (pos, &model_change, flip, JD);

  // when fliped, change sign - most probably work diferently on N and S; tested only on S
  if ((flip && telLatitude < 0) || (!flip && telLatitude > 0))
    model_change.dec *= -1;

  LibnovaRaDec lchange (&model_change);

#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Before model " << ac << dc << lchange <<
    sendLog;
#endif /* DEBUG_EXTRA */

  ac += (CWORD32) (model_change.ra * haCpd);
  dc += (CWORD32) (model_change.dec * decCpd);

#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "After model" << ac << dc << sendLog;
#endif /* DEBUG_EXTRA */

  ac -= homeOff;

  return 0;
}

int
Rts2DevTelParamount::counts2sky (CWORD32 & ac, CWORD32 dc, double &ra,
				 double &dec)
{
  double JD, lst;
  CWORD32 homeOff;
  int ret;

  ret = getHomeOffset (homeOff);
  if (ret)
    return -1;

  JD = ln_get_julian_from_sys ();
  lst = getLstDeg (JD);

  ac += homeOff;

  ra = (double) (ac / haCpd) - haZero;
  dec = (double) (dc / decCpd) - decZero;

  ra = lst - ra;

  // flipped
  if (fabs (dec) > 90)
    {
      telFlip = 1;
      if (dec > 0)
	dec = 180 - dec;
      else
	dec = -180 - dec;
      ra += 180;
      ac += (CWORD32) (RA_TICKS / 2.0);
    }
  else
    {
      telFlip = 0;
    }

  while (ac < acMin)
    ac += RA_TICKS;
  while (ac > acMax)
    ac -= RA_TICKS;

  dec = ln_range_degrees (dec);
  if (dec > 180.0)
    dec -= 360.0;

  ra = ln_range_degrees (ra);

  if (telLatitude < 0)
    dec *= -1;

  return 0;
}

Rts2DevTelParamount::Rts2DevTelParamount (int in_argc, char **in_argv):Rts2DevTelescope (in_argc,
		  in_argv)
{
  axis0.unitId = 0x64;
  axis0.axisId = 0;

  axis1.unitId = 0x64;
  axis1.axisId = 1;

  park_axis[0] = PARK_AXIS0;
  park_axis[1] = PARK_AXIS1;

  device_name = "/dev/ttyS0";
  paramount_cfg = "/etc/rts2/paramount.cfg";
  addOption ('f', "device_name", 1, "device file (default /dev/ttyS0");
  addOption ('P', "paramount_cfg", 1,
	     "paramount config file (default /etc/rts2/paramount.cfg");
  addOption ('R', "recalculate", 1,
	     "track update interval in sec; < 0 to disable track updates; defaults to 1 sec");

  addOption ('D', "dec_park", 1, "DEC park position");
  // in degrees! 30 for south, -30 for north hemisphere 
  // it's (lst - ra(home))
  // haZero and haCpd is handled in ::init, after we get latitude from config file
  haZero = -30.0;
  decZero = 0;

  // how many counts per degree
  haCpd = -32000.0;		// - for N hemisphere, + for S hemisphere
  decCpd = -20883.33333333333;

  acMargin = 10000;

  timerclear (&track_start_time);
  timerclear (&track_recalculate);
  timerclear (&track_next);

  track_recalculate.tv_sec = 1;
  track_recalculate.tv_usec = 0;

  track0 = NULL;
  track1 = NULL;

  // apply all correction for paramount
  corrections = COR_ABERATION | COR_PRECESSION | COR_REFRACTION;

  // int paramout values
  paramountValues.
    push_back (ParaVal ("Index angle", T16, CMD_VAL16_INDEX_ANGLE));
  paramountValues.push_back (ParaVal ("Base rate", T32, CMD_VAL32_BASERATE));
  paramountValues.
    push_back (ParaVal ("Maximum speed", T32, CMD_VAL32_SLEW_VEL));
  paramountValues.
    push_back (ParaVal ("Acceleration", T32, CMD_VAL32_SQRT_ACCEL));
//  paramountValues.push_back (ParaVal ("Non-sidereal tracking rate" ));
  paramountValues.
    push_back (ParaVal
	       ("Minimum position limit", T32, CMD_VAL32_MIN_POS_LIM));
  paramountValues.
    push_back (ParaVal
	       ("Maximum position limit", T32, CMD_VAL32_MAX_POS_LIM));
  paramountValues.
    push_back (ParaVal ("Sensor (from sync)", T16, CMD_VAL16_HOME_SENSORS));
  paramountValues.
    push_back (ParaVal ("Guide", T32, CMD_VAL32_CONSTS_VEL_GUIDE));
//  paramountValues.push_back (ParaVal ("Tics per revolution",
  paramountValues.push_back (ParaVal ("PEC ratio", T16, CMD_VAL16_PEC_RATIO));
  paramountValues.
    push_back (ParaVal
	       ("Maximum position error", T16, CMD_VAL16_ERROR_LIMIT));
//  paramountValues.push_back (ParaVal ("Unit Id.",
  paramountValues.
    push_back (ParaVal ("EMF constants", T16, CMD_VAL16_EMF_FACTOR));
  paramountValues.
    push_back (ParaVal ("Home velocity high", T32, CMD_VAL32_HOMEVEL_HI));
  paramountValues.
    push_back (ParaVal ("Home velocity medium", T32, CMD_VAL32_HOMEVEL_MED));
  paramountValues.
    push_back (ParaVal ("Home velocity low", T32, CMD_VAL32_HOMEVEL_LO));
  paramountValues.
    push_back (ParaVal ("Home direction, sense", T16, CMD_VAL16_HOMEDIR));
  paramountValues.
    push_back (ParaVal
	       ("Home mode, required, Joystick, In-out-in", T16,
		CMD_VAL16_HOME_MODE));
  paramountValues.
    push_back (ParaVal ("Home index offset", T16, CMD_VAL16_HOME2INDEX));
  paramountValues.
    push_back (ParaVal ("PEC cutoff speed", T32, CMD_VAL32_PEC_CUT_SPEED));
  paramountValues.
    push_back (ParaVal ("Maximum voltage", T16, CMD_VAL16_MOTOR_VOLT_MAX));
  paramountValues.
    push_back (ParaVal ("Maximum gain", T16, CMD_VAL16_MOTOR_PROPORT_MAX));
  paramountValues.
    push_back (ParaVal ("Home sense 1", T16, CMD_VAL16_HOMESENSE1));
  paramountValues.
    push_back (ParaVal ("Home sense 2", T16, CMD_VAL16_HOMESENSE2));
  paramountValues.push_back (ParaVal ("Cur pos", T32, CMD_VAL32_CURPOS));
}

Rts2DevTelParamount::~Rts2DevTelParamount (void)
{
  delete track0;
  delete track1;
}

int
Rts2DevTelParamount::processOption (int in_opt)
{
  double rec_sec;
  switch (in_opt)
    {
    case 'f':
      device_name = optarg;
      break;
    case 'P':
      paramount_cfg = optarg;
      break;
    case 'R':
      rec_sec = atof (optarg);
      track_recalculate.tv_sec = (int) rec_sec;
      track_recalculate.tv_usec =
	(int) ((rec_sec - track_recalculate.tv_sec) * USEC_SEC);
      break;
    case 'D':
      park_axis[1] = atoi (optarg);
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelParamount::init ()
{
  CWORD16 motorState0, motorState1;
  CWORD32 pos0, pos1;
  int ret;
  int i;

  Rts2Config *config = Rts2Config::instance ();
  ret = config->loadFile ();
  if (ret)
    return -1;
  telLongtitude = config->getObserver ()->lng;
  telLatitude = config->getObserver ()->lat;

  if (telLatitude < 0)		// south hemispehere
    {
      // swap values which are opposite for south hemispehere
      haZero *= -1.0;
      haCpd *= -1.0;
    }

  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  ret = MKS3Init (device_name);
  if (ret)
    return -1;

  ret = updateLimits ();
  if (ret)
    return -1;

  moveState = TEL_SLEW;

  ret0 = MKS3MotorOn (axis0);
  ret1 = MKS3MotorOn (axis1);
  checkRet ();

  ret0 = MKS3PosAbort (axis0);
  ret1 = MKS3PosAbort (axis1);
  checkRet ();

  // wait till it stop slew
  for (i = 0; i < 10; i++)
    {
      ret = updateStatus ();
      if (ret)
	return ret;
      ret0 = MKS3MotorStatusGet (axis0, &motorState0);
      ret1 = MKS3MotorStatusGet (axis1, &motorState1);
      ret = checkRet ();
      if (ret)
	return ret;
      if (!((motorState0 & MOTOR_SLEWING) || (motorState1 & MOTOR_SLEWING)))
	break;
      if ((motorState0 & MOTOR_POSERRORLIM)
	  || (motorState1 & MOTOR_POSERRORLIM))
	{
	  ret0 = MKS3ConstsMaxPosErrorSet (axis0, 16000);
	  ret1 = MKS3ConstsMaxPosErrorSet (axis1, 16000);
	  ret = checkRet ();
	  if (ret)
	    return ret;
	  i = 10;
	  break;
	}
      sleep (1);
    }

  if (i == 10)
    {
      ret0 = MKS3PosCurGet (axis0, &pos0);
      ret1 = MKS3PosCurGet (axis1, &pos1);
      ret = checkRet ();
      if (ret)
	return ret;
      ret0 = MKS3PosTargetSet (axis0, pos0);
      ret1 = MKS3PosTargetSet (axis1, pos1);
      ret = checkRet ();
      if (ret)
	return ret;
    }

  ret0 = MKS3MotorOff (axis0);
  ret1 = MKS3MotorOff (axis1);
  ret = checkRet ();

  return ret;
}

void
Rts2DevTelParamount::updateTrack ()
{
  double JD;
  struct ln_equ_posn corr_pos;
  double track_delta;
  CWORD32 ac, dc, homeOff;
  MKS3ObjTrackStat stat0, stat1;
  int ret;

  gettimeofday (&track_next, NULL);
  if (track_recalculate.tv_sec < 0)
    return;
  timeradd (&track_next, &track_recalculate, &track_next);

  if (!track0 || !track1)
    {
      JD = ln_get_julian_from_sys ();
      ret = getHomeOffset (homeOff);
      if (ret)
	return;

      getTargetCorrected (&corr_pos, JD);

      ret = sky2counts (&corr_pos, ac, dc, JD, homeOff);
      if (ret)
	return;
#ifdef DEBUG_EXTRA
      logStream (LOG_DEBUG) << "Rts2DevTelParamount::updateTrack " << ac <<
	" " << dc << sendLog;
#endif /* DEBUG_EXTRA */
      ret0 = MKS3PosTargetSet (axis0, (long) ac);
      ret1 = MKS3PosTargetSet (axis1, (long) dc);

      // if that's too far..home us
      if (ret0 == MAIN_AT_LIMIT)
	{
	  ret0 = MKS3Home (axis0, 0);
	  moveState |= TEL_FORCED_HOMING0;
	}
      if (ret1 == MAIN_AT_LIMIT)
	{
	  ret1 = MKS3Home (axis1, 0);
	  moveState |= TEL_FORCED_HOMING1;
	}
      checkRet ();
      return;
    }

  track_delta =
    track_next.tv_sec - track_start_time.tv_sec + (track_next.tv_usec -
						   track_start_time.tv_usec) /
    USEC_SEC;
  JD = ln_get_julian_from_timet (&track_next.tv_sec);
  JD += track_next.tv_usec / USEC_SEC / 86400.0;
  getTargetCorrected (&corr_pos, JD);
  // calculate position at track_next time
  sky2counts (&corr_pos, ac, dc, JD, 0);

#ifdef DEBUG_EXTRA
  logStream (LOG_DEBUG) << "Track ac " << ac << " dc " << dc << " " <<
    track_delta << sendLog;
#endif /* DEBUG_EXTRA */

  ret0 =
    MKS3ObjTrackPointAdd (axis0, track0,
			  track_delta +
			  track0->prevPointTimeTicks / track0->sampleFreq,
			  (CWORD32) (ac + (track_delta * 10000)), &stat0);
  ret1 =
    MKS3ObjTrackPointAdd (axis1, track1,
			  track_delta +
			  track1->prevPointTimeTicks / track1->sampleFreq,
			  (CWORD32) (dc + (track_delta * 10000)), &stat1);
  checkRet ();
}

int
Rts2DevTelParamount::idle ()
{
  struct timeval now;
  CWORD32 homeOff, ac = 0;
  int ret;
  // check if we aren't close to RA limit
  ret = MKS3PosCurGet (axis0, &ac);
  if (ret)
    {
      sleep (10);
      return Rts2Device::idle ();
    }
  ret = getHomeOffset (homeOff);
  if (ret)
    {
      sleep (10);
      return Rts2Device::idle ();
    }
  ac += homeOff;
  if (telLatitude < 0)
    {
      if ((ac + acMargin) > acMax)
	needStop ();
    }
  else
    {
      if ((ac - acMargin) < acMin)
	needStop ();
    }
  ret = updateStatus ();
  if (ret)
    {
      // give mount time to recover
      sleep (10);
      // don't check for move etc..
      return Rts2Device::idle ();
    }
  // issue new track request..if needed and we aren't homing
  if ((getState (0) & TEL_MASK_MOVING) == TEL_OBSERVING
      && !(status0 & MOTOR_HOMING) && !(status0 & MOTOR_SLEWING)
      && !(status1 & MOTOR_HOMING) && !(status1 & MOTOR_SLEWING))
    {
      gettimeofday (&now, NULL);
      if (timercmp (&track_next, &now, <))
	updateTrack ();
    }
  // check for some critical stuff
  return Rts2DevTelescope::idle ();
}

int
Rts2DevTelParamount::baseInfo ()
{
  CWORD16 pMajor, pMinor, pBuild;
  int ret;
  ret = MKS3VersionGet (axis0, &pMajor, &pMinor, &pBuild);
  if (ret)
    return ret;
  snprintf (telType, 64, "Paramount_%i_%i_%i", pMajor, pMinor, pBuild);
  return 0;
}

int
Rts2DevTelParamount::info ()
{
  CWORD32 ac = 0, dc = 0;
  int ret;
  ret0 = MKS3PosCurGet (axis0, &ac);
  ret1 = MKS3PosCurGet (axis1, &dc);
  ret = checkRet ();
  if (ret)
    return ret;
  telSiderealTime = getLocSidTime ();
  ret = counts2sky (ac, dc, telRa, telDec);
  telAxis[0] = ac;
  telAxis[1] = dc;
  if (ret)
    return ret;
  return Rts2DevTelescope::info ();
}

int
Rts2DevTelParamount::startMove (double tar_ra, double tar_dec)
{
  int ret;
  CWORD32 ac = 0;
  CWORD32 dc = 0;

  delete track0;
  delete track1;

  track0 = NULL;
  track1 = NULL;

  ret = updateStatus ();
  if (ret)
    return -1;
  ret0 = MKS_OK;
  ret1 = MKS_OK;
  // when we are homing, we will move after home finish
  if (status0 & MOTOR_HOMING)
    {
      moveState = TEL_FORCED_HOMING0;
      return 0;
    }
  if (status1 & MOTOR_HOMING)
    {
      moveState = TEL_FORCED_HOMING1;
      return 0;
    }
  if ((status0 & MOTOR_OFF) || (status1 & MOTOR_OFF))
    {
      ret0 = MKS3MotorOn (axis0);
      ret1 = MKS3MotorOn (axis1);
      usleep (USEC_SEC / 10);
    }
  ret = checkRet ();
  if (ret)
    {
      return -1;
    }

  ret = sky2counts (tar_ra, tar_dec, ac, dc);
  if (ret)
    {
      return -1;
    }

  moveState = TEL_SLEW;

  ret0 = MKS3PosTargetSet (axis0, (long) ac);
  ret1 = MKS3PosTargetSet (axis1, (long) dc);

  // if that's too far..home us
  if (ret0 == MAIN_AT_LIMIT)
    {
      ret0 = MKS3Home (axis0, 0);
      moveState |= TEL_FORCED_HOMING0;
    }
  if (ret1 == MAIN_AT_LIMIT)
    {
      ret1 = MKS3Home (axis1, 0);
      moveState |= TEL_FORCED_HOMING1;
    }
  return checkRet ();
}

int
Rts2DevTelParamount::isMoving ()
{
  // we were called from idle loop
  if (moveState & (TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1))
    {
      if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
	return USEC_SEC / 10;
      // re-move
      struct ln_equ_posn tar;
      getTargetCorrected (&tar);
      return startMove (tar.ra, tar.dec);
    }
  if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
    return USEC_SEC / 10;
  // we reached destination
  return -2;
}

int
Rts2DevTelParamount::endMove ()
{
  int ret;
//  int ret_track;
  ret0 = MKS3MotorOn (axis0);
  ret1 = MKS3MotorOn (axis1);
  ret = checkRet ();
  // init tracking structures
  gettimeofday (&track_start_time, NULL);
  timerclear (&track_next);
/*  if (!track0)
    track0 = new MKS3ObjTrackInfo ();
  if (!track1)
    track1 = new MKS3ObjTrackInfo ();
  ret0 = MKS3ObjTrackInit (axis0, track0);
  ret1 = MKS3ObjTrackInit (axis1, track1);
  ret_track = checkRet ();
  if (!ret_track)
    updateTrack ();
#ifdef DEBUG_EXTRA
  std::cout << "Track init " << ret0 << " " << ret1 << std::endl;
#endif // DEBUG_EXTRA */
  // 1 sec sleep to get time to settle down
  sleep (1);
  if (!ret)
    return Rts2DevTelescope::endMove ();
  return ret;
}

int
Rts2DevTelParamount::stopMove ()
{
  int ret;
  // if we issue startMove after abor, we will get to position after homing is performed
  if ((moveState & TEL_FORCED_HOMING0) || (moveState & TEL_FORCED_HOMING1))
    return 0;
  // check if we are homing..
  ret = updateStatus ();
  if (ret)
    return -1;
  if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
    return 0;
  ret0 = MKS3PosAbort (axis0);
  ret1 = MKS3PosAbort (axis1);
  return checkRet ();
}

int
Rts2DevTelParamount::startPark ()
{
  int ret;

  delete track0;
  delete track1;

  track0 = NULL;
  track1 = NULL;

  ret0 = MKS3MotorOn (axis0);
  ret1 = MKS3MotorOn (axis1);
  ret = checkRet ();
  if (ret)
    return -1;
  ret0 = MKS3Home (axis0, 0);
  ret1 = MKS3Home (axis1, 0);
  moveState = TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1;
  return checkRet ();
}

int
Rts2DevTelParamount::isParking ()
{
  int ret;
  if (moveState & (TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1))
    {
      if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
	return USEC_SEC / 10;
      moveState = TEL_SLEW;
      // move to park position
      ret0 = MKS3PosTargetSet (axis0, park_axis[0]);
      ret1 = MKS3PosTargetSet (axis1, park_axis[1]);
      ret = updateStatus ();
      if (ret)
	return -1;
      return USEC_SEC / 10;
    }
  // home finished, only check if we get to proper position
  if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
    return USEC_SEC / 10;
  return -2;
}

int
Rts2DevTelParamount::endPark ()
{
  int ret;
  ret = updateStatus ();
  if (ret)
    return -1;
  if (!(status0 & MOTOR_HOMED) || !(status1 & MOTOR_HOMED))
    return -1;
  ret0 = MKS3MotorOff (axis0);
  ret1 = MKS3MotorOff (axis1);
  return checkRet ();
}

int
Rts2DevTelParamount::correct (double cor_ra, double cor_dec, double real_ra,
			      double real_dec)
{
  return correctOffsets (cor_ra, cor_dec, real_ra, real_dec);
}

int
Rts2DevTelParamount::saveModel ()
{
  // dump Paramount stuff
  int ret;
  std::ofstream os (paramount_cfg);
  if (os.fail ())
    return -1;
  ret = saveAxis (os, axis0);
  if (ret)
    return -1;
  ret = saveAxis (os, axis1);
  if (ret)
    return -1;
  os.close ();
  return 0;
}

int
Rts2DevTelParamount::loadModel ()
{
  std::string name;
  std::ifstream is (paramount_cfg);
  MKS3Id *used_axe = NULL;
  int axisId;
  int ret;

  if (is.fail ())
    return -1;
  while (!is.eof ())
    {
      is >> name;
      // find a comment, swap axis
      if (name.find ("**") == 0)
	{
	  is >> name >> axisId;
	  is.ignore (2000, is.widen ('\n'));
	  if (is.fail ())
	    return -1;
	  switch (axisId)
	    {
	    case 0:
	      used_axe = &axis0;
	      break;
	    case 1:
	      used_axe = &axis1;
	      break;
	    default:
	      return -1;
	    }
	}
      else
	{
	  // read until ":"
	  while (true)
	    {
	      if (name.find (':') != name.npos)
		break;
	      std::string name_ap;
	      is >> name_ap;
	      if (is.fail ())
		return -1;
	      name = name + std::string (" ") + name_ap;
	    }
	  name = name.substr (0, name.size () - 1);
	  // find value and load it
	  for (std::vector < ParaVal >::iterator iter =
	       paramountValues.begin (); iter != paramountValues.end ();
	       iter++)
	    {
	      if ((*iter).isName (name))
		{
		  if (!used_axe)
		    return -1;
		  // found value
		  ret = (*iter).readAxis (is, *used_axe);
		  if (ret)
		    return -1;
		  is.ignore (2000, is.widen ('\n'));
		}
	    }
	}
    }
  return 0;
}

Rts2DevTelParamount *device;

int
main (int argc, char **argv)
{
  int ret;
  device = new Rts2DevTelParamount (argc, argv);
  ret = device->init ();
  if (ret)
    return ret;
  ret = device->run ();
  delete device;
  return ret;
}
