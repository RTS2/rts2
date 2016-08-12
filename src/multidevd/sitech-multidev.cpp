/**
 * Driver for multiple Sitech devices
 * Copyright (C) 2016 Petr Kubanek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "multidev.h"
#include "sensord.h"
#include "sitech-focuser.h"
#include "sitech-mirror.h"
#include "connection/sitech.h"

#define OPT_CONFIG_M3     OPT_LOCAL + 1243
#define OPT_CONFIG_FOC    OPT_LOCAL + 1244

SitechMultiBase::SitechMultiBase (int argc, char **argv):rts2core::Daemon (argc, argv)
{
	focName = "F2";
	mirrorName = "M3";
	sitechDev = NULL;
	sitechConn = NULL;

	defaultM3 = NULL;
	defaultFoc = NULL;

	addOption ('f', NULL, 1, "sitech device file");
	addOption (OPT_CONFIG_M3, "defaults-m3", 1, "defaults for device");
	addOption (OPT_CONFIG_FOC, "defaults-foc", 1, "defaults for focuser");
}

SitechMultiBase::~SitechMultiBase ()
{
	delete sitechConn;
}

int SitechMultiBase::run ()
{
	int ret = init ();
	if (ret)
		return ret;
	ret = initHardware ();
	if (ret)
		return ret;
	doDaemonize ();
	return md.run ();
}

int SitechMultiBase::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			sitechDev = optarg;
			break;
		case OPT_CONFIG_M3:
			defaultM3 = optarg;
			break;
		case OPT_CONFIG_FOC:
			defaultFoc = optarg;
			break;
		default:
			return Daemon::processOption (opt);
	}
	return 0;
}

int SitechMultiBase::initHardware ()
{
	if (sitechDev == NULL)
		return -1;

	sitechConn = new rts2core::ConnSitech (sitechDev, this);
	sitechConn->setDebug (getDebug ());
	int ret = sitechConn->init ();
	if (ret)
		return -1;
	sitechConn->flushPortIO ();

	md.push_back (new rts2focusd::SitechFocuser (focName, sitechConn, defaultFoc));
	md.push_back (new rts2mirror::SitechMirror (mirrorName, sitechConn, defaultM3));

	return 0;
}

int main (int argc, char **argv)
{
	SitechMultiBase mb (argc, argv);
	return mb.run ();
}
