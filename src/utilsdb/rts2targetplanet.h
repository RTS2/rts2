#ifndef __RTS2_TARGETPLANET__
#define __RTS2_TARGETPLANET__

#include "target.h"

class TargetPlanet:public Target
{
private:
  int planet_id;		// 0-10
public:
    TargetPlanet (int tar_id, struct ln_lnlat_posn *in_obs);
    virtual ~ TargetPlanet (void);

  virtual int load ();
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);

  virtual int isContinues ();
  virtual void printExtra (std::ostream & _os, double JD);
};

#endif /*! __RTS2_TARGETPLANET__ */
