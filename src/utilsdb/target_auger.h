#ifndef __RTS2_TARGET_AUGER__
#define __RTS2_TARGET_AUGER__

#include "target.h"

class TargetAuger:public ConstTarget
{
private:
  int t3id;
  double auger_date;
  int npixels;
  int augerPriorityTimeout;
public:
    TargetAuger (int in_tar_id, struct ln_lnlat_posn *in_obs,
		 int in_augerPriorityTimeout);
    virtual ~ TargetAuger (void);

  virtual int load ();
  virtual float getBonus (double JD);
  virtual moveType afterSlewProcessed ();
  virtual int considerForObserving (double JD);
  virtual int changePriority (int pri_change, time_t * time_ch)
  {
    // do not drop priority
    return 0;
  }
  virtual int isContinues ()
  {
    return 1;
  }

  virtual void printExtra (std::ostream & _os, double JD);

  virtual void writeToImage (Rts2Image * image);
};

#endif /* !__RTS2_TARGET_AUGER__ */
