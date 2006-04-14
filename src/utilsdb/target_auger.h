#ifndef __RTS2_TARGET_AUGER__
#define __RTS2_TARGET_AUGER__

#include "target.h"

class TargetAuger:public Target
{
private:
  int t3id;
  time_t date;
  int npixels;
  double sdpphi;
  double sdptheta;
  double sdpangle;
public:
    TargetAuger (int in_tar_id, struct ln_lnlat_posn *in_obs);
    virtual ~ TargetAuger (void);

  virtual int load ();
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual float getBonus (double JD);

  virtual void printExtra (std::ostream & _os);
};

#endif /* !__RTS2_TARGET_AUGER__ */
