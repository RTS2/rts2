#ifndef __RTS_TARGET__
#define __RTS_TARGET__
#include <errno.h>
#include <libnova/libnova.h>
#include <stdio.h>
#include <time.h>

#include "status.h"

#include "../utils/objectcheck.h"

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

#define COMMAND_EXPOSURE	"E"
#define COMMAND_FILTER		"F"
#define COMMAND_PHOTOMETER      'P'
#define COMMAND_CHANGE		'C'
#define COMMAND_WAIT            'W'
#define COMMAND_SLEEP		'S'
#define COMMAND_UTC_SLEEP	'U'
#define COMMAND_CENTERING	'c'
#define	COMMAND_FOCUSING	'f'
#define COMMAND_MIRROR		'M'

#define TARGET_FOCUSING		11

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
  int obs_id;
  int img_id;

  int type;			// light, dark, flat, flat_dark
  char obs_type;		// SKY_SURVEY, GBR, .. 

  int getDBScript (int target, const char *camera_name, char *script);
protected:
  int bonus;			// tar_priority + tar_bonus

  int target_id;
  struct ln_lnlat_posn *observer;

  // print nice formated log strings
  void logMsg (const char *message);
  void logMsg (const char *message, int num);
  void logMsg (const char *message, long num);
  void logMsg (const char *message, double num);
  void logMsg (const char *message, const char *val);
  void logMsgDb (const char *message);
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
  virtual int startObservation ();
  virtual int endObservation ();

  virtual int move ();
  virtual int acquire ();
  virtual int observe ();
  virtual int postprocess ();
  // scheduler functions
  virtual int considerForObserving (ObjectCheck * checker, double lst);	// return 0, when target can be observed, otherwise modify tar_bonus..
  virtual int dropBonus ();
  virtual int getBonus ()
  {
    return bonus;
  }
  virtual int changePriority (int pri_change, double validJD);

  virtual int getNextImgId ()
  {
    return ++img_id;
  }
};

class ConstTarget:public Target
{
private:
  struct ln_equ_posn position;
public:
    ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double jd);
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
    virtual int getScript (const char *deviceName, char *buf);
public:
    TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs);
};

// load target from DB
Target *createTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);

#endif /*! __RTS_TARGET__ */
