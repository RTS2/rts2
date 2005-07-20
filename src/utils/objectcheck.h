#ifndef __RTS2__OBJECTCHECK__
#define __RTS2__OBJECTCHECK__

#include <vector>
#include <libnova/ln_types.h>

/**
 * Class for checking, whenewer observation target is correct or no.
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */
class ObjectCheck
{
private:
  std::vector < struct ln_equ_posn >horizont;
  int load_horizont (char *horizont_file);
  inline int is_above_horizont (double st, double dec, double ra1,
				double dec1, double ra2, double dec2);
public:
    ObjectCheck (char *horizont_file);
   ~ObjectCheck ();
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
  int is_good (double lst, double ra, double dec, int hardness = 0);
};

#endif /* ! __RTS2__OBJECTCHECK__ */
