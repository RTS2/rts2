#ifndef __RTS2_TARGET_AUGER__
#define __RTS2_TARGET_AUGER__

#include "target.h"

class TargetAuger:public ConstTarget
{
private:
  int t3id;
  double auger_date;
  int npixels;
public:
    TargetAuger (int in_tar_id, struct ln_lnlat_posn *in_obs);
    virtual ~ TargetAuger (void);

  virtual int load ();
  virtual float getBonus (double JD);
  virtual int considerForObserving (double JD);
  virtual int isContinues ()
  {
    return 1;
  }

  virtual void printExtra (std::ostream & _os, double JD);
};

#endif /* !__RTS2_TARGET_AUGER__ */
