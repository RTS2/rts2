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

#include "filterd.h"

#include "tcsutils.h"

#define OPT_TCSHOST     OPT_LOCAL + 520

namespace rts2filterd
{

/**
 * MDM focuser driver. This is for MDM (Kitt Peak, Arizona) telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MDM:public Filterd
{
	public:
		MDM (int argc, char **argv);
		virtual ~ MDM (void);

	protected:
		virtual int setFilterNum (int new_filter);
		virtual int getFilterNum ();

		virtual int processOption (int opt);
		virtual int init ();

	private:
		int tcssock;
		const char *tcshost;
};

};

using namespace rts2filterd;

MDM::MDM (int argc, char **argv):Filterd (argc, argv)
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
			return Filterd::processOption (opt);
	}
	return 0;
}

int MDM::init ()
{
	int ret;
	ret = Filterd::init ();
	if (ret)
		return ret;
	
	tcssock = miss_opensock_clnt ((char *) tcshost);

	if (tcssock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot init MISS connection" << sendLog;
		return -1;
	}

	return info ();
}

int MDM::setFilterNum (int new_filter)
{
	char buf[255];
	snprintf (buf, 255, "FILTER %d", new_filter + 1);
	std::cout << "tcssock " << tcssock << std::endl;
	int ret = miss_makereq (tcssock, buf, MIS_MSG_REQFILTER, MIS_MSG_FILTER);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set filter number" << sendLog;
		return -1;
	}
	return 0;
}

int MDM::getFilterNum ()
{
	int ret;
	misinfo_t misi;
	ret = miss_reqinfo (tcssock, &misi, 0, 1);
	if (ret < 0)
		return ret;

	return misi.filt;
}

int main (int argc, char **argv)
{
	MDM device = MDM (argc, argv);
	return device.run ();
}
