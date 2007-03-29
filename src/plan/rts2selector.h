#ifndef __RTS2_SELECTOR__
#define __RTS2_SELECTOR__

#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"

class Rts2Selector
{
private:
  std::list < Target * >possibleTargets;
  void considerTarget (int consider_tar_id, double JD);
  void checkTargetObservability ();
  void checkTargetBonus ();
  void findNewTargets ();
  int selectFlats ();
  int selectDarks ();
  struct ln_lnlat_posn *observer;
  double flat_sun_min;
  double flat_sun_max;
public:
    Rts2Selector (struct ln_lnlat_posn *in_observer);
    virtual ~ Rts2Selector (void);
  int selectNext (int masterState);	// return next observation..
  int selectNextNight (int in_bonusLimit = 0);

  double getFlatSunMin ()
  {
    return flat_sun_min;
  }
  double getFlatSunMax ()
  {
    return flat_sun_max;
  }

  void setFlatSunMin (double in_m)
  {
    flat_sun_min = in_m;
  }
  void setFlatSunMax (double in_m)
  {
    flat_sun_max = in_m;
  }
};

#endif /* !__RTS2_SELECTOR__ */
