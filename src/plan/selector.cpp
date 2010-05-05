/*
 * Selector body.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/rts2devclient.h"
#include "rts2selector.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utils/rts2event.h"
#include "../utils/rts2command.h"

#define OPT_IDLE_SELECT         OPT_LOCAL + 5

#define EVENT_SELECT_NEXT       RTS2_LOCAL_EVENT + 5077

class Rts2DevClientTelescopeSel:public rts2core::Rts2DevClientTelescope
{
	protected:
		virtual void moveEnd ();
	public:
		Rts2DevClientTelescopeSel (Rts2Conn * in_connection);
};

Rts2DevClientTelescopeSel::Rts2DevClientTelescopeSel (Rts2Conn * in_connection):rts2core::Rts2DevClientTelescope (in_connection)
{
}

void Rts2DevClientTelescopeSel::moveEnd ()
{
	if (!moveWasCorrecting)
		connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
	rts2core::Rts2DevClientTelescope::moveEnd ();
}


class Rts2DevClientExecutorSel:public rts2core::Rts2DevClientExecutor
{
	protected:
		virtual void lastReadout ();
	public:
		Rts2DevClientExecutorSel (Rts2Conn * in_connection);
};

Rts2DevClientExecutorSel::Rts2DevClientExecutorSel (Rts2Conn * in_connection):rts2core::Rts2DevClientExecutor (in_connection)
{
}

void Rts2DevClientExecutorSel::lastReadout ()
{
	connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
	rts2core::Rts2DevClientExecutor::lastReadout ();
}

class SelectorDev:public Rts2DeviceDb
{
	public:
		SelectorDev (int argc, char **argv);
		virtual ~ SelectorDev (void);

		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		virtual void postEvent (Rts2Event * event);
		virtual int changeMasterState (int new_state);

		int selectNext ();		 // return next observation..
		int updateNext ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

	private:
		rts2plan::Selector * sel;

		Rts2ValueInteger *next_id;

		Rts2ValueInteger *idle_select;
		Rts2ValueInteger *night_idle_select;

		Rts2ValueBool *selEnabled;

		Rts2ValueDouble *flatSunMin;
		Rts2ValueDouble *flatSunMax;

		Rts2ValueString *nightDisabledTypes;
};

SelectorDev::SelectorDev (int argc, char **argv):Rts2DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, "SEL")
{
	sel = NULL;

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

	addOption (OPT_IDLE_SELECT, "idle_select", 1, "selection timeout (reselect every I seconds)");
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
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}

int SelectorDev::reloadConfig ()
{
	int ret;
	struct ln_lnlat_posn *observer;

	ret = Rts2DeviceDb::reloadConfig ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();
	observer = config->getObserver ();

	delete sel;

	sel = new rts2plan::Selector (observer);

	flatSunMin->setValueDouble (sel->getFlatSunMin ());
	flatSunMax->setValueDouble (sel->getFlatSunMax ());

	nightDisabledTypes->setValueCharArr (sel->getNightDisabledTypes ().c_str ());

	deleteTimers (EVENT_SELECT_NEXT);
	addTimer (idle_select->getValueInteger (), new Rts2Event (EVENT_SELECT_NEXT));

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
	return sel->selectNext (getMasterState ());
}

int SelectorDev::updateNext ()
{
	Rts2Conn *exec;
	next_id->setValueInteger (selectNext ());
	if (next_id->getValueInteger () > 0)
	{
		exec = getOpenConnection ("EXEC");
		if (exec && selEnabled->getValueBool ())
		{
			exec->queCommand (new rts2core::Rts2CommandExecNext (this, next_id->getValueInteger ()));
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
