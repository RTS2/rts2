#ifndef __RTS2_TARGET__
#define __RTS2_TARGET__

#include "rts2block.h"

#include <libnova/libnova.h>
#include <time.h>

#define MAX_COMMAND_LENGTH              2000

// move was executed
#define OBS_BIT_MOVED		0x01
// observation started - expect some nice images in db
#define OBS_BIT_STARTED		0x02
// set while in acquisition
#define OBS_BIT_ACQUSITION	0x04
// when observation was interupted
#define OBS_BIT_INTERUPED	0x10
// when acqusition failed
#define OBS_BIT_ACQUSITION_FAI	0x20
// observation was processed
#define OBS_BIT_PROCESSED	0x40

#define TARGET_NAME_LEN		150

#define TYPE_UNKNOW		'u'

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
#define TYPE_MODEL		'm'
#define TYPE_PLANET		'L'

#define TYPE_SWIFT_FOV		'W'
#define TYPE_INTEGRAL_FOV	'I'

#define TYPE_AUGER		'A'

#define TYPE_DARK		'd'
#define TYPE_FLAT		'f'
#define TYPE_FOCUSING		'o'

#define TYPE_LANDOLT		'l'

// master plan target
#define TYPE_PLAN		'p'

// send message when observation stars
#define SEND_START_OBS		0x01
// send message when first image from given observation get astrometry
#define SEND_ASTRO_OK		0x02
// send message at the end of observation
#define SEND_END_OBS		0x04
// send message at end of processing
#define SEND_END_PROC		0x08
// send message at end of night, with all observations and number of images obtained/processed
#define SEND_END_NIGHT		0x10

class Rts2Image;

typedef enum
{
  OBS_MOVE_FAILED = -1,
  OBS_MOVE = 0,
  OBS_ALREADY_STARTED,
  OBS_DONT_MOVE,
  OBS_MOVE_FIXED,
  OBS_MOVE_UNMODELLED
} moveType;

/**
 * This abstract class defines target interface.
 * It's indendet to be used when a target is required.
 *
 * \see Rts2TargetDb, Rts2ScriptTarget
 */
class Rts2Target
{
private:
  int obs_id;
  int moveCount;
  int img_id;			// count for images
  bool tar_enabled;
  // mask with 0xf0 - 0x00 - nominal end 0x10 - interupted 0x20 - acqusition don't converge
  int obs_state;		// 0 - not started 0x01 - slew started 0x02 - images taken 0x04 - acquistion started
  int selected;			// how many times startObservation was called
  int acquired;
  int epochId;
protected:
  int target_id;
  int obs_target_id;
  char target_type;
  char *target_name;
public:
    Rts2Target ()
  {
    obs_id = -1;
    moveCount = 0;
    img_id = 0;
    tar_enabled = true;
    target_id = -1;
    obs_target_id = -1;
    obs_state = 0;
    selected = 0;
    acquired = 0;
  }
  virtual ~ Rts2Target (void)
  {
  }
  // target manipulation functions
  virtual int getScript (const char *device_name, char *buf) = 0;

  int getPosition (struct ln_equ_posn *pos)
  {
    return getPosition (pos, ln_get_julian_from_sys ());
  }

  // return target position at given julian date
  virtual int getPosition (struct ln_equ_posn *pos, double JD) = 0;

  // move functions

  void moveStarted ()
  {
    moveCount = 1;
  }

  void moveEnded ()
  {
    moveCount = 2;
  }

  void moveFailed ()
  {
    moveCount = 3;
  }

  bool moveWasStarted ()
  {
    return (moveCount != 0);
  }

  bool wasMoved ()
  {
    return (moveCount == 2);
  }

  int getCurrImgId ()
  {
    return img_id;
  }

  int getNextImgId ()
  {
    return ++img_id;
  }

  bool getTargetEnabled ()
  {
    return tar_enabled;
  }
  void setTargetEnabled (bool new_en = true)
  {
    if (tar_enabled != new_en)
      {
	logStream (MESSAGE_INFO) << "Target " << getTargetID () << (new_en ?
								    " enabled"
								    :
								    " disabled")
	  << sendLog;
	tar_enabled = new_en;
      }
  }
  virtual int setNextObservable (time_t * time_ch) = 0;
  virtual void setTargetBonus (float new_bonus, time_t * new_time = NULL) = 0;

  virtual int save (bool overwrite) = 0;
  virtual int save (bool overwrite, int tar_id) = 0;

  int getTargetID ()
  {
    return target_id;
  }
  virtual int getObsTargetID ()
  {
    if (obs_target_id > 0)
      return obs_target_id;
    return getTargetID ();
  }

  // called when we can move to next observation - good to generate next target in mosaic observation etc..
  virtual int beforeMove ()
  {
    return 0;
  }

  virtual moveType startSlew (struct ln_equ_posn * position) = 0;
  virtual int startObservation (Rts2Block * master) = 0;

  int getObsId ()
  {
    return obs_id;
  }

  int getSelected ()
  {
    return selected;
  }

  /**
   * Set observation ID and start observation
   */
  void setObsId (int new_obs_id)
  {
    obs_id = new_obs_id;
    selected++;
    obs_state |= OBS_BIT_MOVED;
  }
  int getObsState ()
  {
    return obs_state;
  }

  // called when waiting for acqusition..
  int isAcquired ()
  {
    return (acquired == 1);
  }
  int getAcquired ()
  {
    return acquired;
  }
  void nullAcquired ()
  {
    acquired = 0;
  }

  // return 0 when acquistion isn't running, non 0 when we are currently
  // acquiring target (searching for correct field)
  int isAcquiring ()
  {
    return (obs_state & OBS_BIT_ACQUSITION);
  }
  void obsStarted ()
  {
    obs_state |= OBS_BIT_STARTED;
  }

  virtual void acqusitionStart ()
  {
    obs_state |= OBS_BIT_ACQUSITION;
  }

  void acqusitionEnd ()
  {
    obs_state &= ~OBS_BIT_ACQUSITION;
    acquired = 1;
  }

  void interupted ()
  {
    obs_state |= OBS_BIT_INTERUPED;
  }

  void acqusitionFailed ()
  {
    obs_state |= OBS_BIT_ACQUSITION_FAI;
    acquired = -1;
  }

  virtual void writeToImage (Rts2Image * image) = 0;

  int getEpoch ()
  {
    return epochId;
  }
  void setEpoch (int in_epochId)
  {
    epochId = in_epochId;
  }

  char getTargetType ()
  {
    return target_type;
  }
  void setTargetType (char in_target_type)
  {
    target_type = in_target_type;
  }
  const char *getTargetName ()
  {
    return target_name;
  }
  void setTargetName (const char *in_target_name)
  {
    delete[]target_name;
    target_name = new char[strlen (in_target_name) + 1];
    strcpy (target_name, in_target_name);
  }
};

#endif /* !__RTS2_TARGET__ */
