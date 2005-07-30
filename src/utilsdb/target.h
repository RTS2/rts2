#ifndef __RTS_TARGET__
#define __RTS_TARGET__
#include <errno.h>
#include <libnova/libnova.h>
#include <stdio.h>
#include <time.h>

#include "status.h"

#include "../utils/objectcheck.h"
#include "../utils/rts2device.h"

#define MAX_READOUT_TIME		120
#define PHOT_TIMEOUT			10
#define EXPOSURE_TIMEOUT		50

#define MAX_COMMAND_LENGTH              2000

#define TYPE_OPORTUNITY         'O'
#define TYPE_GRB                'G'
#define TYPE_GRB_TEST		'g'
#define TYPE_SKY_SURVEY         'S'
#define TYPE_GPS                'P'
#define TYPE_ELLIPTICAL         'E'
#define TYPE_HETE               'H'
#define TYPE_PHOTOMETRIC	'M'
#define TYPE_TECHNICAL          't'
#define TYPE_TERESTIAL		'T'
#define TYPE_CALIBRATION	'c'

#define TYPE_SWIFT_FOV		'W'
#define TYPE_INTEGRAL_FOV	'I'

#define TYPE_DARK		'd'
#define TYPE_FLAT		'f'
#define TYPE_FOCUSING		'o'

#define COMMAND_EXPOSURE	"E"
#define COMMAND_DARK		"D"
#define COMMAND_FILTER		"F"
#define COMMAND_PHOTOMETER      'P'
#define COMMAND_CHANGE		"C"
#define COMMAND_WAIT            'W'
#define COMMAND_SLEEP		'S'
#define COMMAND_UTC_SLEEP	'U'
#define COMMAND_CENTERING	'c'
#define	COMMAND_FOCUSING	'f'
#define COMMAND_MIRROR		'M'

#define TARGET_DARK		1
#define TARGET_FLAT		2
#define TARGET_FOCUSING		3

#define TARGET_SWIFT_FOV	10
#define TARGET_INTEGRAL_FOV	11

#define OBS_DONT_MOVE		2

/**
 * Class for one observation.
 *
 * It is created when it's possible to observer such target, and when such
 * observation have some scientific value.
 * 
 * It's put on the observation que inside planc. It's then executed.
 *
 * During execution it can do centering (acquiring images to better know scope position),
 * it can compute importance of future observations.
 *
 * After execution planc select new target for execution, run image processing
 * on first observation target in the que.
 *
 * After all images are processed, new priority can be computed, some results
 * (e.g. light curves) can be put to the database.
 *
 * Target is backed by observation entry in the DB. 
 */
class Target
{
private:
  int epochId;
  int obs_id;
  int img_id;

  int type;			// light, dark, flat, flat_dark
  char obs_type;		// SKY_SURVEY, GBR, .. 
  int selected;			// how many times startObservation was called

  int startCalledNum;		// how many times startObservation was called - good to know for targets
  double airmassScale;

  time_t observationStart;
  // which changes behaviour based on how many times we called them before
protected:
  int target_id;
  struct ln_lnlat_posn *observer;

  virtual int getDBScript (const char *camera_name, char *script);

  // print nice formated log strings
  void logMsg (const char *message);
  void logMsg (const char *message, int num);
  void logMsg (const char *message, long num);
  void logMsg (const char *message, double num);
  void logMsg (const char *message, const char *val);
  void logMsgDb (const char *message);
  virtual int selectedAsGood ();	// get called when target was selected to update bonuses etc..
public:
    Target (int in_tar_id, struct ln_lnlat_posn *in_obs);
    virtual ~ Target (void);

  virtual int getScript (const char *device_name, char *buf);
  int getPosition (struct ln_equ_posn *pos)
  {
    return getPosition (pos, ln_get_julian_from_sys ());
  }
  // return target position at given julian date
  virtual int getPosition (struct ln_equ_posn *pos, double JD)
  {
    return -1;
  }

  // some libnova trivials to get more comfortable access to
  // coordinates
  int getAltAz (struct ln_hrz_posn *hrz)
  {
    return getAltAz (hrz, ln_get_julian_from_sys ());
  }
  virtual int getAltAz (struct ln_hrz_posn *hrz, double JD);

  int getGalLng (struct ln_gal_posn *gal)
  {
    return getGalLng (gal, ln_get_julian_from_sys ());
  }
  virtual int getGalLng (struct ln_gal_posn *gal, double JD);

  double getGalCenterDist ()
  {
    return getGalCenterDist (ln_get_julian_from_sys ());
  }
  double getGalCenterDist (double JD);

  double getAirmass ()
  {
    return getAirmass (ln_get_julian_from_sys ());
  }
  virtual double getAirmass (double JD);

  double getZenitDistance ()
  {
    return getZenitDistance (ln_get_julian_from_sys ());
  }

  double getZenitDistance (double JD);

  double getHourAngle ()
  {
    return getHourAngle (ln_get_julian_from_sys ());
  }

  double getHourAngle (double JD);

  double getDistance (struct ln_equ_posn *in_pos)
  {
    return getDistance (in_pos, ln_get_julian_from_sys ());
  }

  double getDistance (struct ln_equ_posn *in_pos, double JD);

  double getSolarDistance ()
  {
    return getSolarDistance (ln_get_julian_from_sys ());
  }

  double getSolarDistance (double JD);

  double getLunarDistance ()
  {
    return getLunarDistance (ln_get_julian_from_sys ());
  }

  double getLunarDistance (double JD);

  double getMeridianDistance ()
  {
    return fabs (getHourAngle ());
  }

  double getMeridianDistance (double JD)
  {
    return fabs (getHourAngle (JD));
  }

  // time in seconds to object set/rise/meridian pass (if it's visible, otherwise -1 (for
  // circumpolar & not visible objects)
  int secToObjectSet ()
  {
    return secToObjectSet (ln_get_julian_from_sys ());
  }

  int secToObjectSet (double JD);

  int secToObjectRise ()
  {
    return secToObjectRise (ln_get_julian_from_sys ());
  }

  int secToObjectRise (double JD);

  int secToObjectMeridianPass ()
  {
    return secToObjectMeridianPass (ln_get_julian_from_sys ());
  }

  int secToObjectMeridianPass (double JD);

  int getEpoch ()
  {
    return epochId;
  }

  int getTargetID ()
  {
    return target_id;
  }
  int getObsId ()
  {
    return obs_id;
  }
  virtual int getRST (struct ln_rst_time *rst, double jd)
  {
    return -1;
  }
  virtual int startSlew (struct ln_equ_posn *position);
  // return 1 if observation is already in progress, 0 if observation started, -1 on error
  // 2 if we don't need to move
  virtual int startObservation ();
  virtual int endObservation (int in_next_id);
  // similar to startSlew - return 0 if observation ends, 1 if
  // it doesn't ends (ussually in case when in_next_id == target_id),
  // -1 on errror

  virtual int isContinues ()
  {
    return 0;
  }

  virtual int observationStarted ();
  // returns 1 if target is continuus - in case next target is same | next
  // targer doesn't exists, we keep exposing and we will not move mount between
  // exposures. Good for darks observation, partial good for GRB (when we solve
  // problem with moving mount in exposures - position updates)

  virtual int beforeMove ();	// called when we can move to next observation - good to generate next target in mosaic observation etc..
  virtual int acquire ();
  virtual int observe ();
  virtual int postprocess ();
  // scheduler functions
  virtual int considerForObserving (ObjectCheck * checker, double JD);	// return 0, when target can be observed, otherwise modify tar_bonus..
  virtual int dropBonus ();
  virtual float getBonus ()
  {
    return -1;
  }
  virtual int changePriority (int pri_change, time_t * time_ch);
  int changePriority (int pri_change, double validJD);

  virtual int getNextImgId ()
  {
    return ++img_id;
  }

  int getSelected ()
  {
    return selected;
  }

  int getNumObs (time_t * start_time, time_t * end_time);
  double getLastObsTime ();	// return time in seconds to last observation of same target

  int getCalledNum ()
  {
    return startCalledNum;
  }
};

class ConstTarget:public Target
{
private:
  struct ln_equ_posn position;
protected:
  float tar_priority;
  float tar_bonus;

  virtual int selectedAsGood ();	// get called when target was selected to update bonuses etc..
public:
    ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double jd);
  virtual float getBonus ()
  {
    return tar_priority + tar_bonus;
  }
};

class EllTarget:public Target
{
private:
  struct ln_ell_orbit orbit;
public:
    EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double jd);
};

class DarkTarget:public Target
{
private:
  struct ln_equ_posn currPos;
  int defaultDark (const char *deviceName, char *buf);
protected:
    virtual int getScript (const char *deviceName, char *buf);
public:
    DarkTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int startSlew (struct ln_equ_posn *position);
  virtual int isContinues ()
  {
    return 1;
  }
};

class FlatTarget:public ConstTarget
{
protected:
  virtual int getScript (const char *deviceName, char *buf);
public:
    FlatTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int isContinues ()
  {
    return 1;
  }
};

class FocusingTarget:public ConstTarget
{
protected:
  virtual int getScript (const char *deviceName, char *buf);
public:
    FocusingTarget (int in_tar_id,
		    struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id,
							       in_obs)
  {
  };
};

class LunarTarget:public Target
{
protected:
  virtual int getScript (const char *deviceName, char *buf);
public:
    LunarTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double jd);
};

class TargetGRB:public ConstTarget
{
  time_t grbDate;
  time_t lastUpdate;
protected:
    virtual int getDBScript (const char *camera_name, char *script);
  virtual int getScript (const char *deviceName, char *buf);
public:
    TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual float getBonus ();
};

/**
 * Swift Field of View - generate observations on Swift Field of View, based on
 * data received from GCN
 *
 */
class TargetSwiftFOV:public Target
{
private:
  int swiftId;
  struct ln_equ_posn swiftFovCenter;
  time_t swiftTimeStart;
  time_t swiftTimeEnd;
  double swiftOnBonus;
public:
    TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs);
  int findPointing ();		// find Swift pointing for observation
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int startSlew (struct ln_equ_posn *position);
  virtual int considerForObserving (ObjectCheck * checker, double JD);	// return 0, when target can be observed, otherwise modify tar_bonus..
  virtual int beforeMove ();
  virtual float getBonus ();
};

class TargetGps:public ConstTarget
{
public:
  TargetGps (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual float getBonus ();
};

// load target from DB
Target *createTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);

#endif /*! __RTS_TARGET__ */
