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
#include "apm-aux.h"

#define OPT_FILTER         OPT_LOCAL + 50
#define OPT_AUX            OPT_LOCAL + 51
#define OPT_FAN            OPT_LOCAL + 52
#define OPT_BAFFLE         OPT_LOCAL + 53
#define OPT_RELAYS         OPT_LOCAL + 54
#define OPT_TEMPERATURE    OPT_LOCAL + 55

using namespace rts2multidev;

APMMultiBase::APMMultiBase (int argc, char **argv):rts2core::Daemon (argc, argv)
{
	filterName = NULL;
	auxName = NULL;
	filters = NULL;
		
	host = NULL;
	apmConn = NULL;

	hasFan = false;
	hasBaffle = false;
	hasRelays = false;
	hasTemp = false;

	addOption ('e', NULL, 1, "IP and port (separated by :) of the APM box");
	addOption (OPT_FILTER, "filter", 1, "filter wheel device name");
	addOption (OPT_AUX, "aux", 1, "auxiliary device name");
	addOption ('F', NULL, 1, "filter names");
	addOption (OPT_FAN, "fan", 0, "auxiliary device with fan control");
	addOption (OPT_BAFFLE, "baffle", 0, "auxiliary device with baffle (and mirror covers) control");
	addOption (OPT_RELAYS, "relays", 0, "auxiliary device with relay control");
	addOption (OPT_TEMPERATURE, "temperature", 0, "auxiliary device with temperature readout");
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
	return md.run (getDebug ());
}

int APMMultiBase::processOption (int opt)
{
	switch (opt)
	{
		case 'e':
                        host = new HostString (optarg, "5000");
                        break;
		case OPT_FILTER:
			filterName = optarg;
			break;
		case OPT_AUX:
			auxName = optarg;
			break;
		case 'F':
			filters = optarg;
			break;
		case OPT_FAN:
			hasFan = true;
			break;
		case OPT_BAFFLE:
			hasBaffle = true;
			break;
		case OPT_RELAYS:
			hasRelays = true;
			break;
		case OPT_TEMPERATURE:
			hasTemp = true;
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
	{
		rts2filterd::APMFilter *f = new rts2filterd::APMFilter (filterName, apmConn);
		f->setFilters (filters);
		md.push_back (f);
	}

	if (auxName != NULL)
		md.push_back (new rts2sensord::APMAux (auxName, apmConn, hasFan, hasBaffle, hasRelays, hasTemp));

	return 0;
}


int main (int argc, char **argv)
{
	APMMultiBase mb (argc, argv);
	return mb.run ();
}
