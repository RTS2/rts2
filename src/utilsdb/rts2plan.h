#ifndef __RTS2_PLAN__
#define __RTS2_PLAN__

#include "target.h"
#include "rts2obs.h"

#include <ostream>

class Rts2Obs;

class Rts2Plan
{
protected:
  int plan_id;
  int prop_id;
  int tar_id;
  int obs_id;
  time_t plan_start;
  time_t plan_end;

  int plan_status;

  Target *target;
  Rts2Obs *observation;
public:
    Rts2Plan ();
    Rts2Plan (int in_plan_id);
    virtual ~ Rts2Plan (void);
  int load ();
  int save ();
  int del ();

  moveType startSlew (struct ln_equ_posn *position);

  Target *getTarget ();
  void clearTarget ()
  {
    target = NULL;
  }

  Rts2Obs *getObservation ();

  int getPlanId ()
  {
    return plan_id;
  }

  time_t getPlanStart ()
  {
    return plan_start;
  }

  time_t getPlanEnd ()
  {
    return plan_end;
  }

  friend std::ostream & operator << (std::ostream & _os, Rts2Plan * plan);
  friend std::istream & operator >> (std::istream & _is, Rts2Plan & plan);
};

std::ostream & operator << (std::ostream & _os, Rts2Plan * plan);
std::istream & operator >> (std::istream & _is, Rts2Plan & plan);

#endif /* !__RTS2_PLAN__ */
