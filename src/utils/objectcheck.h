#ifndef __RTS2__OBJECTCHECK__
#define __RTS2__OBJECTCHECK__

#include <vector>
#include <libnova/ln_types.h>

/**
 * This holds one value of the horizon file.
 */
class HorizonEntry
{
public:
  struct ln_hrz_posn hrz;

    HorizonEntry (double in_az, double in_alt)
  {
    hrz.az = in_az;
    hrz.alt = in_alt;
  }
};

typedef
std::vector < struct HorizonEntry >
  horizon_t;

/**
 * Class for checking, whenewer observation target is correct or no.
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */
class
  ObjectCheck
{
private:
  enum
  {
    HA_DEC,
    AZ_ALT
  } horType;

  horizon_t
    horizon;
  int
  load_horizon (char *horizon_file);

  double
  getHorizonHeightAz (double az, horizon_t::iterator iter1,
		      horizon_t::iterator iter2);

public:
  ObjectCheck (char *horizon_file);

   ~
  ObjectCheck ();
  /**
   * Check, if that target can be observerd.
   *
   * @param st		local sidereal time of the observation in hours (0-24)
   * @param ra		target ra in deg (0-360)
   * @param dec		target dec
   * @param hardness	how many limits to ignore (Moon distance etc.)
   * 
   * @return 0 if we can observe, <0 otherwise
   */
  int
  is_good (const struct ln_hrz_posn *hrz, int hardness = 0);

  double
  getHorizonHeight (const struct ln_hrz_posn *hrz, int hardness);

  horizon_t::iterator
  begin ()
  {
    return horizon.begin ();
  }

  horizon_t::iterator
  end ()
  {
    return horizon.end ();
  }
};

#endif /* ! __RTS2__OBJECTCHECK__ */
