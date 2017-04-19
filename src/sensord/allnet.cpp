/* 
 * AllNet.de IP temperature sensor.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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
#include "xmlrpc++/XmlRpc.h"

#include <json/json.h>

namespace rts2sensord
{

/**
 * Class for AllNet JSON API IP sensors.
 *
 * JSON API doc: ftp://212.18.29.48/ftp/pub//allnet/switches/JSON/JSON_Anleitungen/json_read_description_0_03en_poe.pdf
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class AllNet:public Sensor
{
	public:
		AllNet (int argc, char **argv);
	
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

	private:
		rts2core::ValueFloat *mirror;
		rts2core::ValueFloat *ambient;

		const char *url;
};

}

using namespace rts2sensord;

AllNet::AllNet (int argc, char **argv):Sensor (argc, argv)
{
	createValue (mirror, "MIRROR", "[C] mirror temperature", true);
	createValue (ambient, "AMBIENT", "[C] ambient temperature", true);

	addOption ('u', NULL, 1, "URL to parse data");

	url = NULL;
}

int AllNet::processOption (int opt)
{
	switch (opt)
	{
		case 'u':
			url = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int AllNet::initHardware ()
{
	if (url == NULL)
	{
		logStream (MESSAGE_ERROR) << "please specify URL to retrieve with -u" << sendLog;
		return -1;
	}
	return 0;
}

int AllNet::info ()
{
	int ret;
	char *reply = NULL;
	int reply_length = 0;

	const char *_uri;

	XmlRpc::XmlRpcClient httpClient (url, &_uri);

	ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "cannot get " << url << sendLog;
		return -1;
	}

	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "received:" << reply << sendLog;

//2017-04-18 21:52:36, CPT, OKTOOPEN, , T, 12.9, WS, 4.4, WD, 79, H, 44.3, DP, 1, PY, 0, BP, 647, WET, FALSE

	struct tm wsdate;


	free (reply);
	setInfoTime (&wsdate);
	return 0;
}

int main (int argc, char **argv)
{
	AllNet device (argc, argv);
	return device.run ();
}
