/* 
 * Cloud sensor which moves.
 * Copyright (C) 2006-2008 Petr Kubanek <petr@kubanek.net>
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
#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Class for cloud sensor which can move.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MrakomerMove:public SensorWeather
{
	private:
		const char cloud_dev;
		rts2core::ConnSerial *cloudConn;

		/**
		 * Read reply from sensor. Check the reply, change parameters accordingly.
		 *
		 * @return -1 on error, 0 when tempereat
		 */
		int readReply (bool temp);

		int cloudHeating ();
		/**
		 * Turn on cloud heating.
		 *
		 * @param cloud heating - 0 - 10 range.
		 */
		int cloudHeating (char step);

		int cloudMeasure (char angle);
		int cloudMeasureAll ();

		void checkCloud ();

		rts2core::ValueSelection *heating;

		rts2core::ValueInteger *ground;
		rts2core::ValueInteger *s45;
		rts2core::ValueInteger *s90;
		rts2core::ValueInteger *s135;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual int idle ();
	public:
		MrakomerMove (int argc, char **argv);
};

}

using namespace rts2sensord;


int
MrakomerMove::readReply (bool temp)
{
	char buf[35];
	int ret;

	int _heating, _ground, _s45, _s90, _s135;

	ret = cloudConn->readPort (buf, 35, '\n');
	if (ret <= 0)
		return -1;
	// parse reply
	ret = sscanf (buf, "G %i;S45 %i;S90 %i;S135 %i", &_ground, &_s45, &_s90, &_s135);
	if (ret == 4)
	{
	  	ground->setValueInteger (_ground);
		s45->setValueInteger (_s45);
		s90->setValueInteger (_s90);
		s135->setValueInteger (_s135);
		return 0;
	}

	ret = sscanf (buf, "H %i;G %i", &_heating, &_ground);
	switch (ret)
	{
		case 2:
			heating->setValueInteger (_heating);
		case 1:
			ground->setValueInteger (_ground);
			if (temp)
				return 1;
			return 0;
	}
	return -1;
}

int
MrakomerMove::cloudHeating (char step)
{
	int ret;

	if (step > 10)
		step = 10;

	ret = cloudConn->writePort ('a' + step);
	if (ret != 1)
		return -1;
	return readReply (false);
}


int
MrakomerMove::cloudHeating ()
{
	char step = 0;
	if (ground->getValueInteger () > 5)
		return 0;
	step += (char) ((-ground->getValueInteger () + 5) / 4.0);
	if (step > 10)
		step = 10;
	return cloudHeating (step);
}


int
MrakomerMove::cloudMeasureAll ()
{
	time_t now;

	int ret;
	ret = cloudConn->writePort ('m');
	if (ret != 1)
		return -1;
	return readReply (false);
}


void
MrakomerMove::checkCloud ()
{
	time_t now;
	time (&now);
	if (now < nextCloudMeas)
		return;

	if (getRain ())
	{
		nextCloudMeas = now + 300;
		return;
	}

	// check that master is in right state..
	switch (getMasterState ())
	{
		case SERVERD_EVENING:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			cloudHeating ();
			cloudMeasureAll ();
								 // TODO doresit dopeni kazdych 10 sec
			nextCloudMeas = now + 60;
			break;
		default:
			cloudHeating ();
			cloudMeasureAll ();
			// 5 minutes mesasurements during the day phase
			nextCloudMeas = now + 300;
	}
}





int
MrakomerMove::processOption (int _opt)
{
	switch (in_opt)
	{
		case 'f':
			cloud_dev = optarg;
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}


int
MrakomerMove::init ()
{
	struct termios newtio;

	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	cloudConn = new Rts2ConnSerial (cloud_dev, this, BS9600, C8, NONE, 40);
	ret = cloudConn->init ();
	if (ret)
	{
		return ret;
	}
}


int
MrakomerMove::idle ()
{
	checkCloud ();
	return SensorWeather::idle ();
}


MrakomerMove::MrakomerMove (int argc, char **argv)
:SensorWeather (argc, argv)
{
	cloud_dev = "/dev/ttyS0";
	cloudConn = NULL;

	createValue (heating, "HEATING", "cloud sensor heating", true, RTS2_VALUE_WRITABLE);
	heating->addSelVal ("0%");
	heating->addSelVal ("10%");
	heating->addSelVal ("20%");
	heating->addSelVal ("30%");
	heating->addSelVal ("40%");
	heating->addSelVal ("50%");
	heating->addSelVal ("60%");
	heating->addSelVal ("70%");
	heating->addSelVal ("80%");
	heating->addSelVal ("90%");
	heating->addSelVal ("100%");

	createValue (ground, "GROUND", "ground temperature", true);
	createValue (s45, "T45", "temperature at 45 deg angle", true);
	createValue (s90, "T90", "temperature at 90 deg angle", true);
	createValue (s135, "T135", "temperature at 135 deg angle", true);

	nextCloudMeas = 0;

	addOption ('f', NULL, 1, "serial port (ussually /dev/ttySn) with the cloud sensor. Default to /dev/ttyS0.");
}


MrakomerMove::~MrakomerMove ()
{
	delete cloudConn;
}


int
main (int argc, char **argv)
{
	MrakomerMove device = MrakomerMove (argc, argv);
	return device.run ();
}
