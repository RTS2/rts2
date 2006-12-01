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
  Rts2TargetSet (struct ln_lnlat_posn *in_obs, bool do_load);
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
  setTargetPriority (float new_priority);
  void
  setTargetBonus (float new_bonus);
  void
  setTargetBonusTime (time_t * new_time);
  void
  setNextObservable (time_t * time_ch);
  void
  setTargetScript (const char *device_name, const char *script);

  int
  save ();
  std::ostream &
  print (std::ostream & _os, double JD);

  std::ostream &
  printBonusList (std::ostream & _os, double JD);
};

class
  Rts2TargetSetSelectable:
  public
  Rts2TargetSet
{
public:
  Rts2TargetSetSelectable (struct ln_lnlat_posn *in_obs = NULL);
};

/**
 * Holds calibration targets
 */
class Rts2TargetSetCalibration:
public Rts2TargetSet
{
public:
  Rts2TargetSetCalibration (Target * in_masterTarget, double JD);
};

/**
 * Holds targets by type
 */
class Rts2TargetSetType:
public Rts2TargetSet
{
public:
  Rts2TargetSetType (char type);
};

class
  TargetGRB;

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
