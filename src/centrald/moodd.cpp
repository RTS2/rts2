/* 
 * Mood sensor - switch all RTS2 systems to soft off, if at least one system is in hard off state.
 * Report state of other centrald.
 * 
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "device.h"

namespace rts2core
{

class MoodD: public Device
{
	private:
		void checkCentralds ();
	public:
		MoodD (int argc, char **argv);

		virtual void centraldConnRunning (Rts2Connection *conn);
		virtual void centraldConnBroken (Rts2Connection *conn);

		virtual int setMasterState (Rts2Connection *_conn, rts2_status_t new_state);
};

}

using namespace rts2core;


void MoodD::checkCentralds ()
{
	connections_t::iterator iter;
	if (getCentraldConns ()->size () == 0)
	{
		setWeatherState (false, "cannot connect to centrald");
		return;
	}

	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if (!((*iter)->isConnState (CONN_CONNECTED) || (*iter)->isConnState (CONN_AUTH_OK)))
		{
			setWeatherState (false, "some centrald connection is not running");
			return;
		}
		if (((*iter)->getState () & SERVERD_ONOFF_MASK) == SERVERD_HARD_OFF)
		{
			setWeatherState (false, "some centrald is in off state");
			return;
		}
	}
	// all connections are up, running, and none is in hard off..
	setWeatherState (true, "all OK");
}

void MoodD::centraldConnRunning (Rts2Connection *conn)
{
	Device::centraldConnRunning (conn);
	checkCentralds ();
}

void MoodD::centraldConnBroken (Rts2Connection *conn)
{
	Device::centraldConnBroken (conn);
 	checkCentralds ();
}

MoodD::MoodD (int argc, char **argv):Device (argc, argv, DEVICE_TYPE_SENSOR, "MOODD")
{

}

int MoodD::setMasterState (Rts2Connection *_conn, rts2_status_t new_state)
{
	Device::setMasterState (_conn, new_state);
	checkCentralds ();
	return 0;
}

int main (int argc, char **argv)
{
	MoodD device = MoodD (argc, argv);
	return device.run ();
}
