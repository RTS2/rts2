/*
 * MDM focuser driver.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "focusd.h"

#include "tcsutils.h"

#define OPT_TCSHOST     OPT_LOCAL + 520

namespace rts2focusd
{

/**
 * MDM focuser driver. This is for MDM (Kitt Peak, Arizona) telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MDM:public Focusd
{
	public:
		MDM (int argc, char **argv);
		virtual ~ MDM (void);

	protected:
		virtual int isFocusing ();
		virtual bool isAtStartPosition ();

		virtual int processOption (int opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (int num);

	private:
		int tcssock;
		const char *tcshost;
};

};

using namespace rts2focusd;

MDM::MDM (int argc, char **argv):Focusd (argc, argv)
{
	tcssock = -1;
	tcshost = "localhost";

	addOption (OPT_TCSHOST, "tcshost", 1, "TCS host name at MDM (default to localhost)");
}


MDM::~MDM (void)
{
	close (tcssock);
}


int MDM::processOption (int opt)
{
	switch (opt)
	{
		case OPT_TCSHOST:
			tcshost = optarg;
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}


int MDM::init ()
{
	int ret;
	ret = Focusd::init ();
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


int MDM::initValues ()
{
	focType = std::string ("MDM focuser");
	return Focusd::initValues ();
}


int MDM::info ()
{
	int ret;
	tcsinfo_t tcsi;
	ret = tcss_reqcoords (tcssock, &tcsi, 0, 1);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "While calling tcss_reqcoords: " << ret << sendLog;
		return ret;
	}

	position->setValueInteger (tcsi.focus);

	return Focusd::info ();
}


int MDM::setTo (int num)
{
	char buf[255];
	snprintf (buf, 255, "FOCUSABS %d", num);
	int ret = tcss_nodata (tcssock, buf, TCS_MSG_REQFOCABS, TCS_MSG_FOCABS);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot set filter number" << sendLog;
		return -1;
	}
	return 0;
}

int MDM::isFocusing ()
{
	return 0;
}


bool MDM::isAtStartPosition ()
{
	return true;
}

int main (int argc, char **argv)
{
	MDM device = MDM (argc, argv);
	return device.run ();
}
