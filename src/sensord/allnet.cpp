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

#include <json.hpp>

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

		rts2core::ValueFloat *mirrorMin;
		rts2core::ValueFloat *mirrorMax;

		const char *url;
};

}

using namespace rts2sensord;

AllNet::AllNet (int argc, char **argv):Sensor (argc, argv)
{
	createValue (mirror, "MIRROR", "[C] mirror temperature", true);
	createValue (ambient, "AMBIENT", "[C] ambient temperature", true);

	createValue (mirrorMin, "md_min", "[C] mirror daily min temperature", false);
	createValue (mirrorMax, "md_max", "[C] mirror daily max temperature", false);

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

	std::ostringstream os;
	os << url << "/xml/json.php?mode=all&simple";

	XmlRpc::XmlRpcClient httpClient (os.str ().c_str (), &_uri);

	ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "cannot get " << url << sendLog;
		return -1;
	}

	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "received:" << reply << sendLog;


	int readed = 0;

	try
	{

// example reply
// [{"id":"1","name":"Internal","unit":"\u00b0C","type":"1","value":"14.26","error":0},{"id":"104","name":"Port 1","unit":"\u00b0C","type":"1","value":"11.56","error":0}]
		auto root = nlohmann::json::parse(reply);

		for (int i = 0; i < (int) root.size(); i++)
		{
			std::string name = root[i]["name"];
			float val = root[i]["value"];

			if (name == "Internal")
			{
				ambient->setValueFloat (val);
				readed |= 0x01;
			}
			else if (name  == "Port 1")
			{
				mirror->setValueFloat (val);
				readed |= 0x02;
			}
			else if (name == "Mirror Min")
			{
				mirrorMin->setValueFloat (val);
			}
			else if (name == "Mirror Max")
			{
				mirrorMax->setValueFloat (val);
			}
		}
	}
	catch (nlohmann::json::exception &ex)
	{
		logStream (MESSAGE_ERROR) << "cannot parse " << reply << sendLog;
		return -1;
	}

	if (readed != 0x03)
		return -1;

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	AllNet device (argc, argv);
	return device.run ();
}
