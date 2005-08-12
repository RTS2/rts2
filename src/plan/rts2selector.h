#ifndef __RTS2_SELECTOR__
#define __RTS2_SELECTOR__

#include "../utilsdb/rts2appdb.h"
#include "../utils/objectcheck.h"
#include "../utilsdb/target.h"

class Rts2Selector
{
private:
  std::list < Target * >possibleTargets;
  void considerTarget (int consider_tar_id, double JD);
  void findNewTargets ();
  int selectFlats ();
  int selectDarks ();
  ObjectCheck *checker;
  struct ln_lnlat_posn *observer;
  double flat_sun_min;
  double flat_sun_max;
public:
    Rts2Selector (struct ln_lnlat_posn *in_observer, char *horizontFile);
    virtual ~ Rts2Selector (void);
  int selectNext (int masterState);	// return next observation..
  int selectNextNight ();
};

#endif /* !__RTS2_SELECTOR__ */
