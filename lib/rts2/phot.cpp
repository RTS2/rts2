/* 
 * Photometer daemon.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "phot.h"
#include "device.h"
#include "status.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>

using namespace rts2phot;

Photometer::Photometer (int in_argc, char **in_argv):ScriptDevice (in_argc, in_argv, DEVICE_TYPE_PHOT, "PHOT")
{
	integrateConn = NULL;

	createValue (filter, "filter", "used filter", false, RTS2_VALUE_WRITABLE);
	createValue (req_count, "required", "number of readings left", false);
	createValue (count, "count", "count of the photometer", false);
	createValue (exp, "exposure", "exposure time in sec", false, RTS2_VALUE_WRITABLE);
	createValue (is_ov, "is_ov", "if photometer overflow", false);

	photType = NULL;

	req_count->setValueInteger (-1);
	setReqTime (1);
}

void Photometer::checkFilterMove ()
{
	long ret;
	if ((getState () & PHOT_MASK_FILTER) == PHOT_FILTER_MOVE)
	{
		ret = isFilterMoving ();
		if (ret > 0)
		{
			setTimeoutMin (ret);
			return;
		}
		// when it's -1 or -2..end filter move
		endFilterMove ();
	}
}

int Photometer::initValues ()
{
	addConstValue ("type", photType);

	return ScriptDevice::initValues ();
}

int Photometer::idle ()
{
	// check filter moving..
	checkFilterMove ();
	return ScriptDevice::idle ();
}

int Photometer::homeFilter ()
{
	return -1;
}

int Photometer::setExposure (float _exp)
{
	setReqTime (_exp);
	return 0;
}

int Photometer::startFilterMove (__attribute__ ((unused)) int new_filter)
{
	maskState (PHOT_MASK_FILTER, PHOT_FILTER_MOVE);
	return 0;
}

long Photometer::isFilterMoving ()
{
	return -2;
}

int Photometer::endFilterMove ()
{
	infoAll ();
	maskState (PHOT_MASK_FILTER, PHOT_FILTER_IDLE);
	return 0;
}

int Photometer::startIntegrate ()
{
	return -1;
}

int Photometer::startIntegrate (rts2core::Connection * conn, float _req_time, int _req_count)
{
	int ret;
	req_count->setValueInteger (_req_count);
	sendValueAll (req_count);
	setReqTime (_req_time);
	ret = startIntegrate ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot start integration");
		return -1;
	}
	maskState (PHOT_MASK_INTEGRATE, PHOT_INTEGRATE, "integration started");
	return 0;
}

int Photometer::endIntegrate ()
{
	maskState (PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE, "integration finished");
	req_count->setValueInteger (-1);
	return 0;
}

int Photometer::stopIntegrate ()
{
	maskState (PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE, "Integration interrupted");
	startIntegrate ();
	return 0;
}

int Photometer::homeFilter (__attribute__ ((unused)) rts2core::Connection * conn)
{
	int ret;
	ret = homeFilter ();
	if (ret)
		return -1;
	filter->setValueInteger (0);
	infoAll ();
	return ret;
}

int Photometer::enableMove ()
{
	return -1;
}

int Photometer::disableMove ()
{
	return -1;
}

int Photometer::moveFilter (int new_filter)
{
	int ret;
	ret = startFilterMove (new_filter);
	if (ret)
		return -1;
	return 0;
}

int Photometer::enableFilter (__attribute__ ((unused)) rts2core::Connection * conn)
{
	int ret;
	ret = enableMove ();
	if (ret)
		return -1;
	infoAll ();
	return 0;
}

int Photometer::scriptEnds ()
{
	stopIntegrate ();
	return ScriptDevice::scriptEnds ();
}

void Photometer::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_NIGHT:
			enableMove ();
			break;
		default:				 /* other - SERVERD_DAY, SERVERD_DUSK, SERVERD_MAINTANCE, SERVERD_OFF, etc */
			disableMove ();
			break;
	}
	ScriptDevice::changeMasterState (old_state, new_state);
}

void Photometer::setReqTime (float in_req_time)
{
	req_time = in_req_time;
	exp->setValueFloat (req_time);
	addTimer (in_req_time, new rts2core::Event (PHOT_EVENT_CHECK, this));
}

void Photometer::postEvent (rts2core::Event *event)
{
	int ret;
	switch (event->getType ())
	{
		case PHOT_EVENT_CHECK:
			ret = getCount ();
			if (ret >= 0 && req_count->getValueInteger () > 0)
				addTimer (ret, new rts2core::Event (PHOT_EVENT_CHECK, this));
			else if (ret < 0)
				endIntegrate ();
			break;	
	}
	ScriptDevice::postEvent (event);
}

int Photometer::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == filter)
		return moveFilter (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == exp)
		return setExposure (new_value->getValueFloat ()) == 0 ? 0 : -2;
	return ScriptDevice::setValue (old_value, new_value);
}

void Photometer::sendCount (int in_count, float in_exp, bool in_is_ov)
{
	count->setValueInteger (in_count);
	exp->setValueFloat (in_exp);
	is_ov->setValueBool (in_is_ov);
	// send value..
	sendValueAll (exp);
	sendValueAll (is_ov);
	sendValueAll (count);
	if (req_count->getValueInteger () > 0)
	{
		req_count->dec();
		sendValueAll (req_count);
	}
	if (req_count->getValueInteger () == 0)
		endIntegrate ();
}

int Photometer::commandAuthorized (rts2core::Connection * conn)
{
	int ret;
	if (conn->isCommand ("home"))
	{
		return homeFilter ();
	}
	else if (conn->isCommand ("integrate"))
	{
		float new_req_time;
		int new_req_count;
		if (conn->paramNextFloat (&new_req_time)
			|| conn->paramNextInteger (&new_req_count) || !conn->paramEnd ())
			return -2;

		return startIntegrate (conn, new_req_time, new_req_count);
	}

	else if (conn->isCommand ("intfil"))
	{
		int new_filter;
		float new_req_time;
		int new_req_count;
		if (conn->paramNextInteger (&new_filter)
			|| conn->paramNextFloat (&new_req_time)
			|| conn->paramNextInteger (&new_req_count) || !conn->paramEnd ())
			return -2;

		ret = moveFilter (new_filter);
		if (ret)
			return ret;
		return startIntegrate (conn, new_req_time, new_req_count);
	}

	else if (conn->isCommand ("stop"))
	{
		return stopIntegrate ();
	}

	else if (conn->isCommand ("filter"))
	{
		int new_filter;
		if (conn->paramNextInteger (&new_filter) || !conn->paramEnd ())
			return -2;
		return moveFilter (new_filter);
	}
	else if (conn->isCommand ("enable"))
	{
		if (!conn->paramEnd ())
			return -2;
		return enableFilter (conn);
	}
	else if (conn->isCommand ("help"))
	{
		conn->sendMsg ("info - phot informations");
		conn->sendMsg ("exit - exit from main loop");
		conn->sendMsg ("help - print, what you are reading just now");
		conn->sendMsg ("integrate <time> <count> - start integration");
		conn->sendMsg ("enable - enable filter movements");
		conn->sendMsg ("stop - stop any running integration");
		return 0;
	}
	return ScriptDevice::commandAuthorized (conn);
}
