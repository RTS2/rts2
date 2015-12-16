/* 
 * Driver for MDM (Kitt Peak, Arizona) telescope.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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
#include "tcsutils.h"

#define OPT_TCSHOST     OPT_LOCAL + 520

namespace rts2teld
{

/**
 * MDM telescope driver. This driver requires MDM API library. Please provide path
 * to it, using --with-mdm option passed to configure call.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MDM:public Telescope
{
	public:
		MDM (int argc, char **argv);
		virtual ~MDM ();

		virtual int startResync ();
		virtual int startMoveFixed (double tar_az, double tar_alt);

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
		int tcssock;
		int missock;
		const char *tcshost;

		rts2core::ValueInteger *probeX;
		rts2core::ValueInteger *probeY;

		int setGP (int x, int y);
};

}

using namespace rts2teld;

MDM::MDM (int argc, char **argv):Telescope (argc, argv)
{
	tcssock = -1;
	missock = -1;

	tcshost = "localhost";

	createValue (probeX, "GP_X", "guider probe X position", true, RTS2_VALUE_WRITABLE);
	createValue (probeY, "GP_Y", "guider probe Y position", true, RTS2_VALUE_WRITABLE);

	addOption (OPT_TCSHOST, "tcshost", 1, "TCS host name at MDM (default to localhost)");
}

MDM::~MDM ()
{
	close (missock);
	close (tcssock);
}

int MDM::startResync ()
{
	struct ln_equ_posn pos;
	getTarget (&pos);
	pos.ra /= 15.0;

	char buf[256];

	snprintf (buf, 255, "SETCOORDS %lf %lf 2000.0", pos.ra, pos.dec);

	int i;
	i = tcss_reqnodata (tcssock, buf, TCS_MSG_REQSENDCOORD, TCS_MSG_SENDCOORD);
	if (i < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set telescope coordinates" << sendLog;
		return -1;
	}

	snprintf (buf, 255, "OFFSET %lf %lf", getCorrRa () * 3600.0, getCorrDec () * 3600.0);

	i = tcss_reqnodata (tcssock, buf, TCS_MSG_REQOFFSET, TCS_MSG_OFFSET);
	if (i < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set telescope offset" << sendLog;
		return -1;
	}

	i = tcss_reqnodata (tcssock, (char *) "COORDGO", TCS_MSG_REQGO, TCS_MSG_GO);
	if (i != 0)
	{
		logStream (MESSAGE_ERROR) << "Telescope Go-To-Coords Failed" << sendLog;
		return -1;
	}

	return 0;
}

int MDM::startMoveFixed (double tar_az, double tar_alt)
{
//	getTarget (&dummyPos);
	return 0;
}

int MDM::stopMove ()
{
	return 0;
}

int MDM::startPark ()
{
	return 0;
}

int MDM::endPark ()
{
	return 0;
}

int MDM::processOption (int opt)
{
	switch (opt)
	{
		case OPT_TCSHOST:
			tcshost = optarg;
			break;
		default:
			return Telescope::processOption (opt);
	}
	return 0;
}

int MDM::initValues ()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	return Telescope::initValues ();
}

int MDM::init ()
{
	int ret;
	ret = Telescope::init ();
	if (ret)
		return ret;
	
	tcssock = tcss_opensock_clnt ((char *) tcshost);	
	if (tcssock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open connection to MDM TCS at " << tcshost << ", error " << strerror (errno) << sendLog;
		return -1;
	}

	missock = miss_opensock_clnt  ((char *) tcshost);
	if (missock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open connection to MDM MIS at " << tcshost << ", error " << strerror (errno) << sendLog;
		return -1;
	}

	return info ();
}

int MDM::info ()
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

int MDM::isMoving ()
{
	return -2;
}

int MDM::isMovingFixed ()
{
	return isMoving ();
}

int MDM::isParking ()
{
	return isMoving ();
}

int MDM::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == probeX)
		return setGP (new_value->getValueInteger (), probeY->getValueInteger ()) < 0 ? -2 : 0;
	else if (old_value == probeY)
		return setGP (probeX->getValueInteger (), new_value->getValueInteger ()) < 0 ? -2 : 0;

	return Telescope::setValue (old_value, new_value);
}

int MDM::setGP (int x, int y)
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
	MDM device (argc, argv);
	return device.run ();
}
