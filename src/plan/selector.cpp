/*
 * Selector body.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "rts2selector.h"
#include "connselector.h"
#include "executorque.h"

#include "../utils/rts2devclient.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utils/rts2event.h"
#include "../utils/rts2command.h"

#define OPT_IDLE_SELECT         OPT_LOCAL + 5
#define OPT_ADD_QUEUE           OPT_LOCAL + 6
#define OPT_FILTERS             OPT_LOCAL + 7
#define OPT_FILTER_FILE         OPT_LOCAL + 8
#define OPT_FILTER_ALIAS        OPT_LOCAL + 9

namespace rts2selector
{

class Rts2DevClientTelescopeSel:public rts2core::Rts2DevClientTelescope
{
	public:
		Rts2DevClientTelescopeSel (Rts2Conn * in_connection):rts2core::Rts2DevClientTelescope (in_connection) {}

	protected:
		virtual void moveEnd ()
		{
			if (!moveWasCorrecting)
				connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
			rts2core::Rts2DevClientTelescope::moveEnd ();
		}
};

class Rts2DevClientExecutorSel:public rts2core::Rts2DevClientExecutor
{
	public:
		Rts2DevClientExecutorSel (Rts2Conn * in_connection):rts2core::Rts2DevClientExecutor (in_connection) {}

	protected:
		virtual void lastReadout ()
		{
			connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
			rts2core::Rts2DevClientExecutor::lastReadout ();
		}
};

class SelectorDev:public Rts2DeviceDb
{
	public:
		SelectorDev (int argc, char **argv);
		virtual ~ SelectorDev (void);

		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		virtual void postEvent (Rts2Event * event);
		virtual int changeMasterState (int new_state);

		int selectNext ();		 // return next observation..

		/**
		 * Update next observation.
		 *
		 * @param started    if true, queue providing last target ID will be informed that the target was observed/is being observed before it will be asked for next ID
		 */
		int updateNext (bool started = false);

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int init ();

	private:
		rts2plan::Selector * sel;

		Rts2ValueInteger *next_id;

		Rts2ValueInteger *idle_select;
		Rts2ValueInteger *night_idle_select;

		Rts2ValueBool *selEnabled;

		Rts2ValueDouble *flatSunMin;
		Rts2ValueDouble *flatSunMax;

		Rts2ValueString *nightDisabledTypes;

		struct ln_lnlat_posn *observer;

		Rts2ValueSelection *selectorQueue;
		Rts2ValueSelection *lastQueue;

		std::list <rts2plan::ExecutorQueue> queues;
		std::list <const char *> queueNames;

		std::list <const char *> filterOptions;
		std::list <const char *> filterFileOptions;
		std::list <const char *> aliasFiles;
};

}

using namespace rts2selector;

SelectorDev::SelectorDev (int argc, char **argv):Rts2DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, "SEL")
{
	sel = NULL;
	observer = NULL;

	selectorQueue = lastQueue = NULL;

	createValue (next_id, "next_id", "ID of next target for selection", false);
	next_id->setValueInteger (-1);

	createValue (idle_select, "idle_select", "interval in seconds in which for selection of next target", false, RTS2_VALUE_WRITABLE | RTS2_DT_INTERVAL);
	idle_select->setValueInteger (300);

	createValue (night_idle_select, "night_idle_select", "interval in seconds for selection of next target during night", false, RTS2_VALUE_WRITABLE | RTS2_DT_INTERVAL);
	night_idle_select->setValueInteger (300);

	createValue (selEnabled, "selector_enabled", "if selector should select next targets", false, RTS2_VALUE_WRITABLE);
	selEnabled->setValueBool (true);

	createValue (flatSunMin, "flat_sun_min", "minimal Solar height for flat selection", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (flatSunMax, "flat_sun_max", "maximal Solar height for flat selection", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);

	createValue (nightDisabledTypes, "night_disabled_types", "list of target types which will not be selected during night", false, RTS2_VALUE_WRITABLE);

	addOption (OPT_IDLE_SELECT, "idle-select", 1, "selection timeout (reselect every I seconds)");
	addOption (OPT_ADD_QUEUE, "add-queue", 1, "add queues with given names; queues will have priority in selection in order they are added");

	addOption (OPT_FILTERS, "available-filters", 1, "available filters for given camera. Camera name is separated with space, filters with :");
	addOption (OPT_FILTER_FILE, "filter-file", 1, "available filter for camera and file separated with :");
	addOption (OPT_FILTER_ALIAS, "filter-aliases", 1, "filter aliases file");
}

SelectorDev::~SelectorDev (void)
{
	delete sel;
}

int SelectorDev::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_IDLE_SELECT:
			idle_select->setValueCharArr (optarg);
			night_idle_select->setValueCharArr (optarg);
			break;
		case OPT_ADD_QUEUE:
			queueNames.push_back (optarg);
			break;
		case OPT_FILTERS:
			filterOptions.push_back (optarg);
			break;
		case OPT_FILTER_FILE:
			filterFileOptions.push_back (optarg);
			break;
		case OPT_FILTER_ALIAS:
			aliasFiles.push_back (optarg);
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}

int SelectorDev::reloadConfig ()
{
	int ret;
	ret = Rts2DeviceDb::reloadConfig ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();
	observer = config->getObserver ();

	delete sel;

	sel = new rts2plan::Selector ();

	sel->setObserver (observer);
	sel->init ();

	std::list <const char *>::iterator iter;

	for (iter = filterOptions.begin (); iter != filterOptions.end (); iter++)
		sel->parseFilterOption (*iter);

	for (iter = filterFileOptions.begin (); iter != filterFileOptions.end (); iter++)
		sel->parseFilterFileOption (*iter);

	for (iter = aliasFiles.begin (); iter != aliasFiles.end (); iter++)
		sel->readAliasFile (*iter);

	flatSunMin->setValueDouble (sel->getFlatSunMin ());
	flatSunMax->setValueDouble (sel->getFlatSunMax ());

	nightDisabledTypes->setValueCharArr (sel->getNightDisabledTypes ().c_str ());

	deleteTimers (EVENT_SELECT_NEXT);
	addTimer (idle_select->getValueInteger (), new Rts2Event (EVENT_SELECT_NEXT));

	return 0;
}

int SelectorDev::init ()
{
	int ret = Rts2DeviceDb::init ();
	if (ret)
		return ret;
	
	if (!queueNames.empty ())
	{
		createValue (selectorQueue, "selector_queue", "type of selector queue mechanism", false, RTS2_VALUE_WRITABLE);
		selectorQueue->addSelVal ("auto", NULL);

		createValue (lastQueue, "last_queue", "queue used for last selection", false);
		lastQueue->addSelVal ("-");
	}
	
	for (std::list <const char *>::iterator iter = queueNames.begin (); iter != queueNames.end (); iter++)
	{
		queues.push_back (rts2plan::ExecutorQueue (this, *iter, &observer));
		selectorQueue->addSelVal (*iter, (Rts2SelData *) &(queues.back ()));
		lastQueue->addSelVal (*iter);
	}
	return 0;
}

rts2core::Rts2DevClient *SelectorDev::createOtherType (Rts2Conn * conn, int other_device_type)
{
	rts2core::Rts2DevClient *ret;
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescopeSel (conn);
		case DEVICE_TYPE_EXECUTOR:
			ret = Rts2DeviceDb::createOtherType (conn, other_device_type);
			updateNext ();
			if (next_id->getValueInteger () > 0 && selEnabled->getValueBool ())
				conn->queCommand (new rts2core::Rts2CommandExecNext (this, next_id->getValueInteger ()));
			return ret;
		default:
			return Rts2DeviceDb::createOtherType (conn, other_device_type);
	}
}

void SelectorDev::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_IMAGE_OK:
			updateNext ();
			break;
		case EVENT_SELECT_NEXT:
			updateNext ();
			addTimer (idle_select->getValueInteger (), event);
			return;
	}
	Rts2DeviceDb::postEvent (event);
}

int SelectorDev::selectNext ()
{
	try
	{
	 	if (selectorQueue)
		{
			int id = -1;
			int q = 1;
			std::list <rts2plan::ExecutorQueue>::iterator iter;
			switch (selectorQueue->getValueInteger ())
			{
				case 0:
					for (iter = queues.begin (); iter != queues.end (); iter++, q++)
					{
						id = iter->selectNextObservation ();
						if (id >= 0)
						{
							lastQueue->setValueInteger (q);
							return id;
						}
					}
					break;
				default:
					rts2plan::ExecutorQueue *eq = (rts2plan::ExecutorQueue *) selectorQueue->getData ();
					if (eq != NULL)
					{
						id = eq->selectNextObservation ();
						if (id >= 0)
						{
							lastQueue->setValueInteger (selectorQueue->getValueInteger ());
							return id;
						}
					}
					break;
			}
			// use selector ass fall-back, if queues are empty
			lastQueue->setValueInteger (0);
		}
		return sel->selectNext (getMasterState ());
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "while selecting next target:" << er << sendLog;
	}
	return -1;
}

int SelectorDev::updateNext (bool started)
{
	if (started)
	{
		if (selectorQueue)
		{
			rts2plan::ExecutorQueue *eq = (rts2plan::ExecutorQueue *) selectorQueue->getData (lastQueue->getValueInteger ());
			if (eq)
			{
				eq->front ().target->startObservation ();
				eq->beforeChange ();
				eq->popFront ();
			}
		}
	}
	next_id->setValueInteger (selectNext ());
	sendValueAll (next_id);
	if (lastQueue)
		sendValueAll (lastQueue);

	if (next_id->getValueInteger () > 0)
	{
		connections_t::iterator iexec = getConnections ()->begin ();  	
		getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iexec);
		if (iexec != getConnections ()->end () && selEnabled->getValueBool ())
		{
			(*iexec)->queCommand (new rts2core::Rts2CommandExecNext (this, next_id->getValueInteger ()));
		}
		return 0;
	}
	return -1;
}

int SelectorDev::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == flatSunMin)
	{
		sel->setFlatSunMin (new_value->getValueDouble ());
		return 0;
	}
	if (old_value == flatSunMax)
	{
		sel->setFlatSunMax (new_value->getValueDouble ());
		return 0;
	}
	if (old_value == nightDisabledTypes)
	{
		sel->setNightDisabledTypes (new_value->getValue ());
		return 0;
	}

	return Rts2DeviceDb::setValue (old_value, new_value);
}

int SelectorDev::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("next"))
	{
		return updateNext () == 0 ? 0 : -2;
	}
	// when observation starts
	else if (conn->isCommand ("observation"))
	{
		return updateNext (true) == 0 ? 0 : -2;
	}
	else if (conn->isCommand ("queue") || conn->isCommand ("queue_at"))
	{
		char *name;
		bool withTimes = conn->isCommand ("queue_at");
		if (conn->paramNextString (&name))
			return -2;
		// try to find queue with name..
		int i = 0;
		std::list <rts2plan::ExecutorQueue>::iterator qi = queues.begin ();
		std::list <const char *>::iterator iter;
		for (iter = queueNames.begin (); iter != queueNames.end () && qi != queues.end (); iter++, i++, qi++)
		{
			if (strcmp (*iter, name) == 0)
				break;
		}
		if (iter == queueNames.end ())
			return -2;
		rts2plan::ExecutorQueue * q = &(*qi);
		return q->queueFromConn (conn, withTimes) == 0 ? 0 : -2;
	}
	else
	{
		return Rts2DeviceDb::commandAuthorized (conn);
	}
}

int SelectorDev::changeMasterState (int new_master_state)
{
	switch (new_master_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_DAWN:
		case SERVERD_DUSK:
			// low latency select to catch right moment for flats
			idle_select->setValueInteger (30);
			sendValueAll (idle_select);
		case SERVERD_MORNING:
		case SERVERD_MORNING | SERVERD_STANDBY:
		case SERVERD_SOFT_OFF:
		case SERVERD_HARD_OFF:
			selEnabled->setValueBool (true);
			break;
		case SERVERD_NIGHT:
			idle_select->setValueInteger (night_idle_select->getValueInteger ());
			sendValueAll (idle_select);
			break;
	}
	updateNext ();
	return Rts2DeviceDb::changeMasterState (new_master_state);
}

int main (int argc, char **argv)
{
	SelectorDev selector (argc, argv);
	return selector.run ();
}
