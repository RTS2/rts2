#ifndef __RTS2_OBS_DB__
#define __RTS2_OBS_DB__

#include <ostream>
#include <time.h>
#include <vector>

#include "../utils/timestamp.h"

#include "rts2count.h"
#include "rts2imgset.h"

// what to print..
#define DISPLAY_ALL		0x01
#define DISPLAY_SUMMARY		0x02

class Rts2ImgSet;

/**
 * Observation class.
 *
 * This class contains observation. They are (optionaly) linked to images.
 * It can calculate (and display) various statistic on images it contains.
 *
 * @author petr
 */
class Rts2Obs
{
private:
  //! list of images for that observation
  Rts2ImgSet imgset;
  std::vector < Rts2Count > counts;

  int displayImages;
  int displayCounts;

  int tar_id;
  int obs_id;
  double obs_ra;
  double obs_dec;
  double obs_alt;
  double obs_az;
  double obs_slew;
  double obs_start;
  int obs_state;
  double obs_end;

public:
  /**
   * Create class from DB
   */
    Rts2Obs (int in_obs_id);
  /**
   * Create class from informations provided.
   *
   * Handy when creating class from cursor.
   */
    Rts2Obs (int in_tar_id, int in_obs_id, double in_obs_ra,
	     double in_obs_dec, double in_obs_alt, double in_obs_az,
	     double in_obs_slew, double in_obs_start, int in_obs_state,
	     double in_obs_end);
    virtual ~ Rts2Obs (void);
  int load ();
  int loadImages ();
  void printImages (std::ostream & _os);
  void printImagesSummary (std::ostream & _os);

  int loadCounts ();
  void printCounts (std::ostream & _os);
  void printCountsSummary (std::ostream & _os);

  void setPrintImages (int in_printImages)
  {
    displayImages = in_printImages;
  }

  void setPrintCounts (int in_printCounts)
  {
    displayCounts = in_printCounts;
  }

  int getTargetId ()
  {
    return tar_id;
  }

  int getObsId ()
  {
    return obs_id;
  }

  friend std::ostream & operator << (std::ostream & _os, Rts2Obs & obs);
};

std::ostream & operator << (std::ostream & _os, Rts2Obs & obs);

class Rts2ObsState
{
private:
  int state;
public:
    Rts2ObsState (int in_state)
  {
    state = in_state;
  }

  friend std::ostream & operator << (std::ostream & _os,
				     Rts2ObsState obs_state);
};

std::ostream & operator << (std::ostream & _os, Rts2ObsState obs_state);

#endif /* !__RTS2_OBS_DB__ */
