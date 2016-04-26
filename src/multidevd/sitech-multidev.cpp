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

SitechMultiBase::SitechMultiBase (int argc, char **argv):rts2core::Daemon (argc, argv)
{
	focName = "F0";
	mirrorName = "M0";
	sitechDev = NULL;
	sitechConn = NULL;

	addOption ('f', NULL, 1, "sitech device file");
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

	return md.run ();
}

int SitechMultiBase::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			sitechDev = optarg;
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

	md.push_back (new rts2focusd::SitechFocuser (focName, sitechConn));
	md.push_back (new rts2mirror::SitechMirror (mirrorName, sitechConn));

	return 0;
}

int main (int argc, char **argv)
{
	SitechMultiBase mb (argc, argv);
	return mb.run ();
}
