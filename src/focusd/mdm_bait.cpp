/* 
 * MDM BAIT 1.3m focuser driver
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

#include "focusd.h"
#include "../../lib/rts2/connbait.h"
#include "tcsutils.h"

#define OPT_TCSHOST     OPT_LOCAL + 520
#define OPT_BAITHOST    OPT_LOCAL + 600

namespace rts2focusd
{

/**
 * MDM BAIT 1.3m focuser.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class BAIT:public Focusd
{
	public:
		BAIT (int argc, char **argv);
		virtual ~BAIT ();

	protected:
		virtual bool isAtStartPosition ();

		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int setTo (double num);
		virtual double tcOffset () { return 0.; }

	private:
		rts2core::ValueFloat *t_tcs, *t_west, *t_frame, *t_mirror, *t_secondary;
		
		HostString *baitServer;
		rts2core::BAIT *connBait;

		int tcssock;
		const char *tcshost;
};

}

using namespace rts2focusd;

BAIT::BAIT (int argc, char **argv):Focusd (argc, argv)
{
	baitServer = NULL;
	connBait = NULL;

	tcssock = -1;
	tcshost = "localhost";

	createValue (t_tcs, "tem_tcs", "[C] TCS temperature", true);
	createValue (t_west, "tem_west", "[C] west temperature", true);
	createValue (t_frame, "tem_fram", "[C] frame temperature", true);
	createValue (t_mirror, "tem_mirr", "[C] mirror temperature", true);
	createValue (t_secondary, "tem_sec", "[C] secondary mirror temperature", true);

	addOption (OPT_BAITHOST, "bait-server", 1, "BAIT server hostname (and port)");
	addOption (OPT_TCSHOST, "tcshost", 1, "TCS host name at MDM (default to localhost)");
}

BAIT::~BAIT ()
{
	close (tcssock);
}

bool BAIT::isAtStartPosition ()
{
	return true;
}

int BAIT::processOption (int opt)
{
	switch (opt)
	{
		case OPT_BAITHOST:
			baitServer = new HostString (optarg, "4928");
			break;
		case OPT_TCSHOST:
			tcshost = optarg;
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}

int BAIT::init ()
{
	int ret = Focusd::init ();
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

	tcssock = tcss_opensock_clnt ((char *) tcshost);	
	if (tcssock < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open connection to MDM TCS at " << tcshost << ", error " << strerror (errno) << sendLog;
		return -1;
	}
	
	return info ();
}

int BAIT::info ()
{
	float t_t, t_w, t_f, t_m, t_s;
	static char buf[500];

	tcsinfo_t tcsi;
	int ret = tcss_reqcoords (tcssock, &tcsi, 0, 1);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "While calling tcss_reqcoords: " << ret << sendLog;
		return ret;
	}

	position->setValueInteger (tcsi.focus);

	connBait->writeRead ("temps", buf, 500);

	ret = sscanf (buf, "done temps tcs=%f west=%f frame=%f mirror=%f secondary=%f", &t_t, &t_w, &t_f, &t_m, &t_s);
	if (ret != 5)
	{
		logStream (MESSAGE_ERROR) << "cannot parse server output: " << buf << " " << ret << sendLog;
		return -1;
	}
	t_tcs->setValueFloat (t_t);
	t_west->setValueFloat (t_w);
	t_frame->setValueFloat (t_f);
	t_mirror->setValueFloat (t_m);
	t_secondary->setValueFloat (t_s);

	return Focusd::info ();
}

int BAIT::setTo (double num)
{
	char buf[255];
	snprintf (buf, 255, "FOCUSABS %d", (int) num);
	int ret = tcss_reqnodata (tcssock, buf, TCS_MSG_REQFOCABS, TCS_MSG_FOCABS);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set focuser " << ret << " " << tcssock << sendLog;
		return -1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	BAIT device (argc, argv);
	return device.run ();
}
