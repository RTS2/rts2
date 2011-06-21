/* 
 * Driver for MDM 1.3m (Kitt Peak, Arizona) telescope.
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "teld.h"
#include "../utils/rts2config.h"
#include "../utils/connbait.h"

#define OPT_BAITHOST     OPT_LOCAL + 521

namespace rts2teld
{

/**
 * MDM 1.3m BAIT (Berkeley Automated Imagining Telescope) driver.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class BAIT:public Telescope
{
	public:
		BAIT (int argc, char **argv);
		virtual ~BAIT ();

		virtual int startResync ();

		virtual int stopMove ();

		virtual int startPark ();

		virtual int endPark ();

	protected:
		virtual int processOption (int opt);

		virtual int initValues ();
		virtual int init ();
		
		virtual int info ();
		virtual int isMoving ();
		virtual int isMovingFixed ();
		virtual int isParking ();

	private:
		HostString *baitServer;
		rts2core::BAIT *connBait;
};

}

using namespace rts2teld;

BAIT::BAIT (int argc, char **argv):Telescope (argc, argv)
{
	baitServer = NULL;
	connBait = NULL;

	addOption (OPT_BAITHOST, "bait-server", 1, "BAIT server hostname (and port)");
}

BAIT::~BAIT ()
{
	delete connBait;
}

int BAIT::startResync ()
{
	struct ln_equ_posn pos;
	getTarget (&pos);
	char buf[256];

	snprintf (buf, 255, "point ra=%f dec=%f decimal equinox=2000 nodome", pos.ra, pos.dec);
	connBait->writeRead (buf, buf, 256);

	return 0;
}

int BAIT::stopMove ()
{
	return 0;
}

int BAIT::startPark ()
{
	char buf[256];
	connBait->writeRead ("home dome", buf, 256);
	return 0;
}

int BAIT::endPark ()
{
	return 0;
}

int BAIT::processOption (int opt)
{
	switch (opt)
	{
		case OPT_BAITHOST:
			baitServer = new HostString (optarg, "4928");
			break;
		default:
			return Telescope::processOption (opt);
	}
	return 0;
}

int BAIT::initValues ()
{
	Rts2Config *config;
	config = Rts2Config::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	strcpy (telType, "MDM 1.3m");
	return Telescope::initValues ();
}

int BAIT::init ()
{
	int ret;
	ret = Telescope::init ();
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
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	
	return info ();
}

int BAIT::info ()
{
	char buf[500];
	connBait->writeRead ("where decimal", buf, 500);
	double ra, dec, equinox, ha, secz, alt, az;
	int ret = sscanf (buf, "done where ra=%lf dec=%lf equinox=%lf ha=%lf secz=%lf alt=%lf az=%lf", &ra, &dec, &equinox, &ha, &secz, &alt, &az);
	if (ret != 7)
	{
		logStream (MESSAGE_ERROR) << "cannot parse BAIT reply: " << buf << sendLog;
		return -1;
	}

	setTelRaDec (ra, dec);
	
	return Telescope::info ();
}

int BAIT::isMoving ()
{
	return -2;
}

int BAIT::isMovingFixed ()
{
	return isMoving ();
}

int BAIT::isParking ()
{
	return isMoving ();
}

int main (int argc, char **argv)
{
	BAIT device (argc, argv);
	return device.run ();
}
