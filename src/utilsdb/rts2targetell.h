#ifndef __RTS2_TARGETELL__
#define __RTS2_TARGETELL__

#include "target.h"

/**
 * Represent target of body moving on elliptical orbit.
 */
class EllTarget:public Target
{
private:
  struct ln_ell_orbit orbit;
  int getPosition (struct ln_equ_posn *pos, double JD,
		   struct ln_equ_posn *parallax);
public:
    EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
    EllTarget ():Target ()
  {
  };
  virtual int load ();
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);

  virtual void printExtra (std::ostream & _os, double JD);

  virtual void writeToImage (Rts2Image * image, double JD);

  double getEarthDistance (double JD);
  double getSolarDistance (double JD);
};

#endif /* !__RTS2_TARGETELL__ */
