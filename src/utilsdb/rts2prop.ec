#include "../utils/rts2config.h"
#include "rts2prop.h"

using namespace rts2db;

void Rts2Prop::init ()
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

Rts2Prop::Rts2Prop (int in_prop_id)
{

}

Rts2Prop::~Rts2Prop ()
{
	delete target;
	delete planSet;
}

int Rts2Prop::load ()
{
	return 0;
}

int Rts2Prop::save ()
{
	return 0;
}

Target * Rts2Prop::getTarget ()
{
	if (target)
		return target;
	return createTarget (tar_id, Rts2Config::instance ()->getObserver());
}

PlanSet * Rts2Prop::getPlanSet ()
{
	if (planSet)
		return planSet;
	return new PlanSet (prop_id);
}
