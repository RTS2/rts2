#ifndef __RTS2_TARGET__
#define __RTS2_TARGET__

/**
 * This completly abstract class defines target interface.
 * It's indendet to be used when a target is required.
 *
 * \see Rts2TargetDb, Rts2ScriptTarget
 */

#include <libnova/libnova.h>
#include <time.h>

#define MAX_COMMAND_LENGTH              2000

class Rts2Target
{
private:
  int moveCount;
  int img_id;			// count for images
  bool tar_enabled;
public:
    Rts2Target ()
  {
    moveCount = 0;
    img_id = 0;
    tar_enabled = true;
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

  virtual int getObsTargetID () = 0;

  virtual void acqusitionStart () = 0;

  virtual int isAcquired () = 0;

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
    tar_enabled = new_en;
  }
  virtual int setNextObservable (time_t * time_ch) = 0;
  virtual void setTargetBonus (float new_bonus, time_t * new_time = NULL) = 0;

  virtual int save (bool overwrite) = 0;
  virtual int save (bool overwrite, int tar_id) = 0;
};

#endif /* !__RTS2_TARGET__ */
