/**
 * Driver for multiple APM device
 * Copyright (C) 2015 Stanislav Vitek
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

#include "apm-multidev.h"
#include "apm-filter.h"
#include "apm-mirror.h"
#include "apm-relay.h"

using namespace rts2multidev;

APMMultiBase::APMMultiBase (int argc, char **argv):rts2core::Daemon (argc, argv)
{
	filterName = "FW0";
	relayName = "R0";
	coversName = "MC0";
	fanName = "FAN0";
		
	host = NULL;
	apmConn = NULL;

	addOption ('e', NULL, 1, "IP and port (separated by :) of APM box");
}

APMMultiBase::~APMMultiBase ()
{
	delete apmConn;
}

int APMMultiBase::run ()
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

int APMMultiBase::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
                        host = new HostString (optarg, "5000");
                        break;

		default:
			return Daemon::processOption (opt);
	}
	return 0;
}

int APMMultiBase::initHardware ()
{
	if (host == NULL)
	{
                logStream (MESSAGE_ERROR) << "You must specify filter hostname (with -e option)." << sendLog;
		return -1;
	}

	apmConn = new rts2core::ConnAPM (host->getPort (), this, host->getHostname ());
	apmConn->setDebug (getDebug ());
	
	int ret = apmConn->init ();
	if (ret)
		return ret;

	if (filterName != NULL)
		md.push_back (new rts2filterd::APMFilter (filterName, apmConn));

	if (relayName != NULL)
		md.push_back (new rts2sensord::APMRelay (relayName, apmConn));

	if (coversName != NULL)
		md.push_back (new rts2sensord::APMMirror (coversName, apmConn));

	return 0;
}


int main (int argc, char **argv)
{
	APMMultiBase mb (argc, argv);
	return mb.run ();
}
