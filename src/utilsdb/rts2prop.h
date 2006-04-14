#ifndef __RTS2_PROP__
#define __RTS2_PROP__

#include "target.h"
#include "rts2planset.h"

#include <iostream>

class Rts2Prop
{
private:
  int prop_id;
  int tar_id;
  int user_id;

  Target *target;
  Rts2PlanSet *planSet;
  void init ();
public:
    Rts2Prop ();
    Rts2Prop (int in_prop_id);
    virtual ~ Rts2Prop (void);

  int load ();
  int save ();

  Target *getTarget ();
  Rts2PlanSet *getPlanSet ();

  friend std::ostream & operator << (std::ostream & _os, Rts2Prop prop);
};

std::ostream & operator << (std::ostream & _os, Rts2Prop prop);

#endif /* !__RTS2_PROP__ */
