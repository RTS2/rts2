/*
 * Executor body.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "executorque.h"

using namespace rts2plan;

ExecutorQueue::ExecutorQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer)
{
  	master = _master;
	std::string sn (name);
	observer = _observer;
	master->createValue (nextIds, (sn + "_ids").c_str (), "next queue IDs", false, RTS2_VALUE_WRITABLE);
	master->createValue (nextNames, (sn + "_names").c_str (), "next queue names", false);
	master->createValue (queueType, (sn + "_queing").c_str (), "queing mode", false, RTS2_VALUE_WRITABLE);

	queueType->addSelVal ("FIFO");
	queueType->addSelVal ("CIRCULAR");
}

ExecutorQueue::~ExecutorQueue ()
{
	clearNext (NULL);
	updateVals ();
}

void ExecutorQueue::updateVals ()
{
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		_id_arr.push_back ((*iter)->getTargetID ());
		_name_arr.push_back ((*iter)->getTargetName ());
	}
	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
}

int ExecutorQueue::addFront (rts2db::Target *nt)
{
	push_front (nt);
	updateVals ();
	return 0;
}

int ExecutorQueue::addTarget (rts2db::Target *nt)
{
	push_back (nt);
	updateVals ();
	return 0;
}

void ExecutorQueue::beforeChange ()
{
	struct ln_hrz_posn altaz;
	switch (queueType->getValueInteger ())
	{
		case QUEUE_FIFO:
			break;
		case QUEUE_CIRCULAR:
			front ()->getAltAz (&altaz);
			if (front ()->isAboveHorizon (&altaz))
			{
				push_back (createTarget (front ()->getTargetID (), *observer));
			}
			else
			{
			  	logStream (MESSAGE_INFO) << "removing target " << front ()->getTargetName () << " (" << front ()->getTargetID () << ") from queue, as it is bellow horizon" << sendLog;
			}
			pop_front ();
			break;
	}
	updateVals ();
}

void ExecutorQueue::popFront ()
{
	switch (queueType->getValueInteger ())
	{
		case QUEUE_FIFO:
			pop_front ();
			break;
		case QUEUE_CIRCULAR:
			break;
	}
	updateVals ();
}

void ExecutorQueue::clearNext (rts2db::Target *currentTarget)
{
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		if (*iter != currentTarget)
			delete *iter;
	}
	clear ();
}


