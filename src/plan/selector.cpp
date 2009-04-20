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

class Rts2DevClientTelescopeSel:public Rts2DevClientTelescope
{
	protected:
		virtual void moveEnd ();
	public:
		Rts2DevClientTelescopeSel (Rts2Conn * in_connection);
};

Rts2DevClientTelescopeSel::Rts2DevClientTelescopeSel (Rts2Conn * in_connection):Rts2DevClientTelescope
(in_connection)
{
}


void
Rts2DevClientTelescopeSel::moveEnd ()
{
	if (!moveWasCorrecting)
		connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
	Rts2DevClientTelescope::moveEnd ();
}


class Rts2DevClientExecutorSel:public Rts2DevClientExecutor
{
	protected:
		virtual void lastReadout ();
	public:
		Rts2DevClientExecutorSel (Rts2Conn * in_connection);
};

Rts2DevClientExecutorSel::Rts2DevClientExecutorSel (Rts2Conn * in_connection):Rts2DevClientExecutor
(in_connection)
{
}


void
Rts2DevClientExecutorSel::lastReadout ()
{
	connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
	Rts2DevClientExecutor::lastReadout ();
}


class Rts2SelectorDev:public Rts2DeviceDb
{
	private:
		Rts2Selector * sel;

		time_t last_selected;

		int next_id;

		Rts2ValueInteger *idle_select;
		Rts2ValueBool *selEnabled;

		Rts2ValueDouble *flatSunMin;
		Rts2ValueDouble *flatSunMax;

		Rts2ValueString *nightDisabledTypes;

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();
	public:
		Rts2SelectorDev (int argc, char **argv);
		virtual ~ Rts2SelectorDev (void);
		virtual int idle ();

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);
		virtual void postEvent (Rts2Event * event);
		virtual int changeMasterState (int new_state);

		int selectNext ();		 // return next observation..
		int updateNext ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
};

Rts2SelectorDev::Rts2SelectorDev (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_SELECTOR, "SEL")
{
	sel = NULL;
	next_id = -1;
	time (&last_selected);

	createValue (idle_select, "idle_select", "time in seconds in which at least one selection will be performed", false);
	idle_select->setValueInteger (300);

	createValue (selEnabled, "selector_enabled", "if selector should select next targets", false);
	selEnabled->setValueBool (true);

	createValue (flatSunMin, "flat_sun_min", "minimal Solar height for flat selection", false, RTS2_DT_DEGREES);
	createValue (flatSunMax, "flat_sun_max", "maximal Solar height for flat selection", false, RTS2_DT_DEGREES);

	createValue (nightDisabledTypes, "night_disabled_types", "list of target types which will not be selected during night", false);

	addOption (OPT_IDLE_SELECT, "idle_select", 1, "selection timeout (reselect every I seconds)");
}


Rts2SelectorDev::~Rts2SelectorDev (void)
{
	delete sel;
}


int
Rts2SelectorDev::processOption (int in_opt)
{
	int t_idle;
	switch (in_opt)
	{
		case OPT_IDLE_SELECT:
			t_idle = atoi (optarg);
			idle_select->setValueInteger (t_idle);
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2SelectorDev::reloadConfig ()
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

	sel = new Rts2Selector (observer);

	flatSunMin->setValueDouble (sel->getFlatSunMin ());
	flatSunMax->setValueDouble (sel->getFlatSunMax ());

	nightDisabledTypes->setValueCharArr (sel->getNightDisabledTypes ().c_str ());

	return 0;
}


int
Rts2SelectorDev::idle ()
{
	time_t now;
	time (&now);
	if (now > last_selected + idle_select->getValueInteger ())
	{
		updateNext ();
		time (&last_selected);
	}
	return Rts2DeviceDb::idle ();
}


Rts2DevClient *
Rts2SelectorDev::createOtherType (Rts2Conn * conn, int other_device_type)
{
	Rts2DevClient *ret;
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescopeSel (conn);
		case DEVICE_TYPE_EXECUTOR:
			ret = Rts2DeviceDb::createOtherType (conn, other_device_type);
			updateNext ();
			if (next_id > 0 && selEnabled->getValueBool ())
				conn->queCommand (new Rts2CommandExecNext (this, next_id));
			return ret;
		default:
			return Rts2DeviceDb::createOtherType (conn, other_device_type);
	}
}


void
Rts2SelectorDev::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_IMAGE_OK:
			updateNext ();
			break;
	}
	Rts2DeviceDb::postEvent (event);
}


int
Rts2SelectorDev::selectNext ()
{
	return sel->selectNext (getMasterState ());
}


int
Rts2SelectorDev::updateNext ()
{
	Rts2Conn *exec;
	next_id = selectNext ();
	if (next_id > 0)
	{
		exec = getOpenConnection ("EXEC");
		if (exec && selEnabled->getValueBool ())
		{
			exec->queCommand (new Rts2CommandExecNext (this, next_id));
		}
		return 0;
	}
	return -1;
}


int
Rts2SelectorDev::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == selEnabled || old_value == idle_select)
	{
		return 0;
	}
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


int
Rts2SelectorDev::changeMasterState (int new_master_state)
{
	switch (new_master_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_MORNING:
		case SERVERD_MORNING | SERVERD_STANDBY:
		case SERVERD_SOFT_OFF:
		case SERVERD_HARD_OFF:
			selEnabled->setValueBool (true);
			break;
	}
	updateNext ();
	return Rts2DeviceDb::changeMasterState (new_master_state);
}


int
main (int argc, char **argv)
{
	Rts2SelectorDev selector = Rts2SelectorDev (argc, argv);
	return selector.run ();
}
