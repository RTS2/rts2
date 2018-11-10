/* 
 * MDM BAIT weather sensor.
 * Copyright (C) 2011 Petr Kubanek, Insitute of Physics <kubanek@fzu.cz>
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

#include "sensord.h"
#include "connection/bait.h"

#define OPT_BAITHOST    OPT_LOCAL + 600

namespace rts2sensord
{

/**
 * MDM BAIT weather sensor. Read and display various weather and enviromentla
 * data.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class BAIT:public Sensor
{
	public:
		BAIT (int argc, char **argv);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();
	private:
		rts2core::ValueInteger *winddir;
		rts2core::ValueFloat *windspeed;
		rts2core::ValueFloat *windpeek;
		rts2core::ValueFloat *oat;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *pressure;
		rts2core::ValueFloat *rainint;
		rts2core::ValueBool *rain;

		// cloud sensor
		rts2core::ValueFloat *ovolts;
		rts2core::ValueFloat *irvolts;
		rts2core::ValueFloat *cloud;
		rts2core::ValueFloat *dew;

	private:
		HostString *baitServer;
		rts2core::BAIT *connBait;
};

}

using namespace rts2sensord;

BAIT::BAIT (int argc, char **argv):Sensor (argc, argv)
{
	baitServer = NULL;
	connBait = NULL;

	createValue (winddir, "winddir", "[deg] wind direction", true);
	createValue (windspeed, "windspeed", "[m/s] wind speed", true);
	createValue (windpeek, "windpeek", "[m/s] peak wind speed", true);
	createValue (oat, "oat", "[C] outside air temperature", true);
	createValue (humidity, "humidity", "[%] relative humidity", true);
	createValue (pressure, "pressure", "[hPa] barometric pressure", true);
	createValue (rainint, "rainint", "rain intensity", true);
	createValue (rain, "rain", "rain (true/false)", true);

	createValue (ovolts, "ovolts", "[V] cloud sensor output", true);
	createValue (irvolts, "irvolts", "[V] infrared sensor reading of the cloud detector", true);
	createValue (cloud, "cloud", "cloud temperature", true);
	createValue (dew, "dew", "dry lower", true);

	addOption (OPT_BAITHOST, "bait-server", 1, "BAIT server hostname (and port)");
}

int BAIT::processOption (int opt)
{
	switch (opt)
	{
		case OPT_BAITHOST:
			baitServer = new HostString (optarg, "4928");
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int BAIT::init ()
{
	int ret = Sensor::init ();
	if (ret)
		return ret;

	if (baitServer == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify BAIT server host with --bait-server option" << sendLog;
		return -1;
	}

	try
	{
		std::string bs = baitServer->getHostname ();
		connBait = new rts2core::BAIT (this, bs, baitServer->getPort ());
		ret = connBait->init ();
	}
	catch (rts2core::ConnError &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	
	return info ();
}

int BAIT::info ()
{
	int wd, ra;
	float ws, wp, ot, hu, ba, raint, co, ci, cl, de;
	static char buf[500];
	connBait->writeRead ("mets", buf, 500);

	int ret = sscanf (buf, "done mets winddir=%i windspeed=%f peak=%f oat=%f humidity=%f bar=%f rainintensity=%f rain=%i", &wd, &ws, &wp, &ot, &hu, &ba, &raint, &ra);
	if (ret != 8)
	{
		logStream (MESSAGE_ERROR) << "cannot parse server output: " << buf << " " << ret << sendLog;
		return -1;
	}
	winddir->setValueInteger (wd);
	windspeed->setValueFloat (ws);
	windpeek->setValueFloat (wp);
	oat->setValueFloat (ot);
	humidity->setValueFloat (hu);
	pressure->setValueFloat (ba);
	rainint->setValueFloat (raint);
	rain->setValueBool (! (ra == 0));

	connBait->writeRead ("taux", buf, 500);
	ret = sscanf (buf, "done taux ovolts=%f irvolts=%f cloud=%f dew=%f", &co, &ci, &cl, &de);
	if (ret != 4)
	{
		logStream (MESSAGE_ERROR) << "cannot parse server output: " << buf << " " << ret << sendLog;
		return -1;
	}
	ovolts->setValueFloat (co);
	irvolts->setValueFloat (ci);
	cloud->setValueFloat (cl);
	dew->setValueFloat (de);

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	BAIT device (argc, argv);
	return device.run ();
}
