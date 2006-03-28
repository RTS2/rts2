#ifndef __RTS2_TELMODEL__
#define __RTS2_TELMODEL__
/**
 * This class describe telescope model interface.
 *
 * Model takes care of converting sky coordinates send to teld to coordinates that
 * will be send to telescope. It doesn't handle issues such as moving targets
 * (bodies in solar system) and proper motions of stars.
 *
 * It can include calculation of refraction, depending on site location and
 * actual athmospheric conditions.
 * 
 * @author petr
 */

#include "../telescope.h"
#include "../../utils/rts2config.h"

#include "modelterm.h"

#include <libnova/libnova.h>

#include <vector>
#include <iostream>

class Rts2ModelTerm;

/**
 * Conditions for model calculation.
 *
 * Holds only conditions which are static, e.g. it will not hold alt&az, as
 * those will be changed in course of model calculation. Holds mount
 * geographics position, current time etc.
 */

class Rts2ObsConditions
{
private:
  Rts2DevTelescope * telescope;
public:
  Rts2ObsConditions (Rts2DevTelescope * in_telescope)
  {
    telescope = in_telescope;
  }
  int getFlip ()
  {
    return telescope->getFlip ();
  }
  double getLatitude ()
  {
    return telescope->getLatitude ();
  }
};

/**
 * Holds telescope model.
 * Performs on terms apply and reverse calls.
 *
 * When we pass ln_equ_posn, ra is hour angle (in degrees), not RA value.
 */
class Rts2TelModel
{
private:
  Rts2ObsConditions * cond;
  const char *modelFile;

  char caption[81];		// Model description: 80 chars + NULL
  char method;			// method: T or S
  int num;			// Number of active observations
  double rms;			// sky RMS (arcseconds)
  double refA;			// refraction constant A (arcseconds)
  double refB;			// refraction constant B (arcseconds)
    std::vector < Rts2ModelTerm * >terms;
public:
    Rts2TelModel (Rts2DevTelescope * in_telescope, const char *in_modelFile);
    virtual ~ Rts2TelModel (void);
  int load ();
  /**
   * Apply model to coordinates. Pos.ra is hour angle, not RA.
   */
  int apply (struct ln_equ_posn *pos);
  int applyVerbose (struct ln_equ_posn *pos);

  int reverse (struct ln_equ_posn *pos);
  int reverse (struct ln_equ_posn *pos, double sid);

  friend std::istream & operator >> (std::istream & is, Rts2TelModel * model);
  friend std::ostream & operator << (std::ostream & os, Rts2TelModel * model);
};

std::istream & operator >> (std::istream & is, Rts2TelModel * model);
std::ostream & operator << (std::ostream & os, Rts2TelModel * model);

#endif /* !__RTS2_TELMODEL__ */
