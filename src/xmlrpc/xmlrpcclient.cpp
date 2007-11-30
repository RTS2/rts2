/* 
 * XML-RPC client.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2007 Stanislav Vitek <standa@iaa.es>
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

#include "xmlrpc++/XmlRpc.h"
#include "../utils/rts2cliapp.h"
#include <iostream>
using namespace XmlRpc;

#define OPT_HOST             OPT_LOCAL + 1

class Rts2XmlRpcTest: public Rts2CliApp
{
	private:
		int xmlPort;
		char *xmlHost;
		int xmlVerbosity;

		XmlRpcClient* xmlClient;

		int runXmlMethod (const char* methodName, XmlRpcValue &args, XmlRpcValue &result, bool printRes = true);
		void doTest ();
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int doProcessing ();

	public:
		Rts2XmlRpcTest (int argc, char **argv);
		virtual ~Rts2XmlRpcTest (void);
};

int
Rts2XmlRpcTest::runXmlMethod (const char* methodName, XmlRpcValue &args, XmlRpcValue &result, bool printRes)
{
	int ret;

	ret = xmlClient->execute (methodName, args, result);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error calling '" << methodName << "'." << sendLog;
		return -1;
	}
	if (printRes)
	{
		std::cout << "Output of method '" << methodName << "':" << std::endl << result << std::endl << std::endl;
	}
	return 0;
}


void
Rts2XmlRpcTest::doTest ()
{
	XmlRpcValue noArgs, oneArg, result;

	runXmlMethod ("system.listMethods", noArgs, result);

	oneArg[0] = "DeviceCount";
	runXmlMethod ("system.methodHelp", oneArg, result);

	runXmlMethod ("system.listDevices", noArgs, result);

	runXmlMethod ("system.listValues", noArgs, result);

	oneArg[0] = "SD";
	runXmlMethod ("system.listValuesDevice", oneArg, result, false);
	std::cout << "listValues for device " << oneArg[0] << std::endl;
	for (int i = 0; i < result.size(); i++)
	{
		std::cout << " " << result[i]["name"] << "=" << result[i]["value"] << std::endl;
	}

	oneArg[0] = "f";
	runXmlMethod ("system.listTargets", oneArg, result);

	oneArg[0] = 4;
	oneArg[1] = 2;
	runXmlMethod ("system.targetInfo", oneArg, result);

	oneArg[0] = 2600;
	runXmlMethod ("system.listImages", oneArg, result);
}


int
Rts2XmlRpcTest::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_PORT:
			xmlPort = atoi (optarg);
			break;
		case OPT_HOST:
			xmlHost = optarg;
			break;
		case 'v':
			xmlVerbosity++;
			break;
		default:
			return Rts2CliApp::processOption (in_opt);
	}
	return 0;
}


int
Rts2XmlRpcTest::doProcessing ()
{
	doTest ();
	return 0;
}


int
Rts2XmlRpcTest::init ()
{
	int ret;
	ret = Rts2CliApp::init ();
	if (ret)
		return ret;
	XmlRpc::setVerbosity(xmlVerbosity);
	xmlClient = new XmlRpcClient (xmlHost, xmlPort);
	return 0;
}


Rts2XmlRpcTest::Rts2XmlRpcTest (int in_argc, char **in_argv): Rts2CliApp (in_argc, in_argv)
{
	xmlPort = 8889;
	xmlHost = "localhost";
	xmlVerbosity = 0;

	xmlClient = NULL;

	addOption (OPT_PORT, "port", 1, "port of XML-RPC server");
	addOption (OPT_HOST, "hostname", 1, "hostname of XML-RPC server");
	addOption ('v', NULL, 0, "verbosity (multiple -v to increase it");
}


Rts2XmlRpcTest::~Rts2XmlRpcTest (void)
{
	delete xmlClient;
}


int main(int argc, char** argv)
{
	Rts2XmlRpcTest app (argc, argv);
	return app.run();
}


// Call system.listVariables method
/*	if (c.execute("system.listValues", noArgs, result))
	{
		std::cout << "Devices + values: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}*/
// Call system.listVariables method with one device
/*if (c.execute("system.listValuesDevice", oneArg, result))
{
	std::cout << "Devices + values for andor: " << std::endl;
	for (int i = 0; i < result.size(); i++)
	{
		std::cout << " " << result[i]["name"] << "=" << result[i]["value"] << std::endl;
	}
}
else
{
	std::cout << "Error calling 'system.listValuesDevice'\n\n";
}*/
