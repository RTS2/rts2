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
protected:
  void
  load (std::string in_where, std::string order_by);
  void
  load (std::list < int >&target_ids);

  struct ln_lnlat_posn *
    obs;
public:
  // print all targets..
  Rts2TargetSet (struct ln_lnlat_posn *in_obs = NULL);
  Rts2TargetSet (struct ln_equ_posn *pos, double radius,
		 struct ln_lnlat_posn *in_obs = NULL);
  Rts2TargetSet (std::list < int >&tar_ids, struct ln_lnlat_posn *in_obs =
		 NULL);
  virtual ~
  Rts2TargetSet (void);

  void
  setTargetEnabled (bool enabled = true);
  void
  setTargetBonus (float new_bonus);
  void
  setTargetBonusTime (time_t * new_time);

  int
  save ();
};

/**
 * Holds calibration targets
 */
class Rts2TargetSetCal:
public Rts2TargetSet
{
public:
  Rts2TargetSetCal (Target * in_masterTarget, double JD);
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
