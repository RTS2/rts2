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
#include "../utils/rts2config.h"
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
	private:
		int tcssock;
		const char *tcshost;
};

}

using namespace rts2teld;

MDM::MDM (int argc, char **argv):Telescope (argc, argv)
{
	tcssock = -1;
	tcshost = "localhost";

	addOption (OPT_TCSHOST, "tcshost", 1, "TCS host name at MDM (default to localhost)");
}

MDM::~MDM ()
{
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
/*	i = tcss_reqnodata (tcssock, buf, TCS_MSG_REQSENDCOORD, TCS_MSG_SENDCOORD);
	if (i < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set telescope coordinates" << sendLog;
		return -1;
	} */

	snprintf (buf, 255, "OFFSET %lf %lf", getCorrRa () / 15.0, getCorrDec ());

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
	Rts2Config *config;
	config = Rts2Config::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	strcpy (telType, "MDM2.4m");
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

int
main (int argc, char **argv)
{
	MDM device = MDM (argc, argv);
	return device.run ();
}
