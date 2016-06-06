/* 
 * WebSocket daemon.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "rts2-config.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/devicedb.h"
#else
#include "device.h"
#endif

#ifdef RTS2_HAVE_PGSQL
class WsD:public rts2db::DeviceDb
#else
class WsD:public rts2core::Device
#endif
{
	public:
		WsD (int argc, char **argv);
		virtual ~WsD ();

	protected:
#ifndef RTS2_HAVE_PGSQL
		virtual int willConnect (NetworkAddress * _addr);
#endif
};

#ifdef RTS2_HAVE_PGSQL
WsD::WsD (int argc, char **argv):rts2db::DeviceDb (argc, argv, DEVICE_TYPE_XMLRPC, "WSD")
#else
WsD::WsD (int argc, char **argv):rts2core::Device (argc, argv, DEVICE_TYPE_XMLRPC, "WSD")
#endif
{
}

WsD::~WsD()
{
}

#ifndef RTS2_HAVE_PGSQL
int WsD::willConnect (NetworkAddress *_addr)
{
	if (_addr->getType () < getDeviceType ()
		|| (_addr->getType () == getDeviceType ()
		&& strcmp (_addr->getName (), getDeviceName ()) < 0))
		return 1;
	return 0;
}
#endif

int main (int argc, char **argv)
{
	WsD device (argc, argv);
	return device.run ();
}
