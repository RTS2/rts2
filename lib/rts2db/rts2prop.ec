/*
 * Target observing proposals.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "configuration.h"
#include "rts2prop.h"
#include "rts2db/sqlerror.h"

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

Rts2Prop::Rts2Prop (__attribute__ ((unused)) int in_prop_id)
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
	try
	{
		target = createTarget (tar_id, rts2core::Configuration::instance ()->getObserver(), rts2core::Configuration::instance ()->getObservatoryAltitude ());
	}
	catch (SqlError &err)
	{
	  	logStream (MESSAGE_ERROR) << "error while retrieving target for plan: " << err << sendLog;
		return NULL;
	}
	return target;
}

PlanSet * Rts2Prop::getPlanSet ()
{
	if (planSet)
		return planSet;
	PlanSet *p = new PlanSet (prop_id);
	p->load ();
	return p;
}
