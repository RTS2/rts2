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
#include "configuration.h"
#include "connection/bait.h"
#include "tcsutils.h"

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

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		HostString *baitServer;
		rts2core::BAIT *connBait;

		int tcssock;
		int missock;

		rts2core::ValueInteger *probeX;
		rts2core::ValueInteger *probeY;

		int setGP (int x, int y);
};

}

using namespace rts2teld;

BAIT::BAIT (int argc, char **argv):Telescope (argc, argv)
{
	baitServer = NULL;
	connBait = NULL;

	tcssock = -1;
	missock = -1;

	createValue (probeX, "GP_X", "guider probe X position", true, RTS2_VALUE_WRITABLE);
	createValue (probeY, "GP_Y", "guider probe Y position", true, RTS2_VALUE_WRITABLE);

	addOption (OPT_BAITHOST, "bait-server", 1, "BAIT server hostname (and port)");
}

BAIT::~BAIT ()
{
	close (missock);
	close (tcssock);

	delete connBait;
}

int BAIT::startResync ()
{
	char buf[500];
	struct ln_equ_posn pos;

	if (!originChangedFromLastResync () && getTargetDistance () > 1)
	{
		// match target and telescope if there is a huge difference between tel and target
		logStream (MESSAGE_WARNING) << "matching telescope and target coordinates," << sendLog;

		connBait->writeRead ("where decimal equinox=2000", buf, 500);
		double ra, dec, equinox, ha, secz, alt, az;
		int ret = sscanf (buf, "done where ra=%lf dec=%lf equinox=%lf ha=%lf secz=%lf alt=%lf az=%lf", &ra, &dec, &equinox, &ha, &secz, &alt, &az);
		if (ret != 7)
		{
			logStream (MESSAGE_ERROR) << "cannot parse BAIT reply: " << buf << sendLog;
			return -1;
		}
		setOrigin (ra, dec);
		pos.ra = ra;
		pos.dec = dec;
		applyOffsets (&pos);
		setTarget (pos.ra, pos.dec);
	}
	else
	{
		getTarget (&pos);
	}

	snprintf (buf, 255, "point ra=%f dec=%f decimal equinox=2000", pos.ra, pos.dec);
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
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	setTelAltitude (config->getObservatoryAltitude ());
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

	tcssock = tcss_opensock_clnt ((char *) "mcgraw");	
	if (tcssock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open connection to MDM TCS at mcgraw, error " << strerror (errno) << sendLog;
		return -1;
	}

	missock = miss_opensock_clnt  ((char *) "mcgraw");
	if (missock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open connection to MDM MIS at mcgraw error " << strerror (errno) << sendLog;
		return -1;
	}
	
	return info ();
}

int BAIT::info ()
{
	int ret;
	tcsinfo_t tcsi;
	ret = tcss_reqcoords (tcssock, &tcsi, 0, 1);
	if (ret < 0)
		return ret;

	setTelRaDec (tcsi.ra * 15.0, tcsi.dec);

	misinfo_t misi;
	ret = miss_reqinfo (missock, &misi, 0, 1);
	if (ret < 0)
		return ret;

	probeX->setValueInteger (misi.guidex);
	probeY->setValueInteger (misi.guidey);

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

int BAIT::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == probeX)
		return setGP (new_value->getValueInteger (), probeY->getValueInteger ()) < 0 ? -2 : 0;
	else if (old_value == probeY)
		return setGP (probeX->getValueInteger (), new_value->getValueInteger ()) < 0 ? -2 : 0;

	return Telescope::setValue (old_value, new_value);
}

int BAIT::setGP (int x, int y)
{
	char pos[256];
	if (x < 0 || x > 25000 || y < 0 || y > 25000)
	{
		logStream (MESSAGE_ERROR) << "invalid probe position " << x << " " << y << sendLog;
		return -1;
	}
	snprintf (pos, 255, "PMOVE %#3.1f %#3.1f", (float) x, (float) y);
	return miss_makereq (missock, pos, MIS_MSG_REQPMOVE, MIS_MSG_PMOVE);
}

int main (int argc, char **argv)
{
	BAIT device (argc, argv);
	return device.run ();
}
