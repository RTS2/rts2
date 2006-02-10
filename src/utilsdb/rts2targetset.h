#ifndef __RTS2_TARGETSET__
#define __RTS2_TARGETSET__

#include <libnova/libnova.h>
#include <list>
#include <ostream>

#include "target.h"

class Rts2TargetSet:public
  std::list <
Target * >
{
private:
  void
  load (std::string in_where, std::string order_by);
protected:
  struct ln_lnlat_posn *
    obs;
public:
  Rts2TargetSet (struct ln_equ_posn *pos, double radius,
		 struct ln_lnlat_posn *in_obs = NULL);
  virtual ~
  Rts2TargetSet (void);
};

/**
 * Holds last GRBs
 */
class Rts2TargetSetGrb:
public std::list < TargetGRB * >
{
private:
  void
  load ();
protected:
  struct ln_lnlat_posn *
    obs;
public:
  Rts2TargetSetGrb (struct ln_lnlat_posn *in_obs = NULL);
  virtual ~ Rts2TargetSetGrb (void);

  void
  printGrbList (std::ostream & _os);
};

std::ostream & operator << (std::ostream & _os, Rts2TargetSet & tar_set);

#endif /* !__RTS2_TARGETSET__ */
