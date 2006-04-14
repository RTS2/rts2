#include "../utils/rts2config.h"
#include "rts2prop.h"

void
Rts2Prop::init ()
{
  prop_id = -1;
  tar_id = -1;
  user_id = -1;

  target = NULL;
  planSet = NULL;
}

Rts2Prop::Rts2Prop ()
{

}

Rts2Prop::Rts2Prop (int prop_id)
{

}

Rts2Prop::~Rts2Prop ()
{
  delete target;
  delete planSet;
}

int
Rts2Prop::load ()
{

}

int
Rts2Prop::save ()
{

}

Target *
Rts2Prop::getTarget ()
{
  if (target)
    return target;
  return createTarget (tar_id, Rts2Config::instance ()->getObserver());
}

Rts2PlanSet *
Rts2Prop::getPlanSet ()
{
  if (planSet)
    return planSet;
  return new Rts2PlanSet (prop_id);
}

std::ostream & operator << (std::ostream & _os, Rts2Prop prop)
{

}
