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
#include <vector>
#include <string.h>

#include "r2x.h"

using namespace XmlRpc;

#define OPT_HOST             OPT_LOCAL + 1
#define OPT_SCHED_TICKET     OPT_LOCAL + 2

namespace rts2xmlrpc
{

/**
 * Class for testing XML-RPC.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Client: public Rts2CliApp
{
	private:
		int xmlPort;
		const char *xmlHost;
		int xmlVerbosity;

		int schedTicket;
		enum {TEST, SET_VARIABLE, SCHED_TICKET, GET_VARIABLE} xmlOp;

		std::vector <const char *> args;

		XmlRpcClient* xmlClient;

		/**
		 * Run one XML-RPC method. Prints out method execution
		 * processing according to verbosity level.
		 *
		 * @param methodName   Name of the method.
		 * @param in           Input arguments.
		 * @param result       Result of execution.
		 * @param printRes     Prints out result.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int runXmlMethod (const char* methodName, XmlRpcValue &in, XmlRpcValue &result, bool printRes = true);

		/**
		 * Run test methods.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int doClient ();

		/**
		 * Set XML-RPC variable.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setVariable (const char *deviceName, const char *varName, const char *value);

		/**
		 * Prints informations about given ticket.
		 *
		 * @param ticketId ID of ticket to print.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int schedTicketInfo (int ticketId);

		/**
		 * Return variables in args.
		 */
		int getVariables ();
	protected:
		virtual void usage ();

		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();

	public:
		Client (int argc, char **argv);
		virtual ~Client (void);
};

};

using namespace rts2xmlrpc;

int
Client::runXmlMethod (const char* methodName, XmlRpcValue &in, XmlRpcValue &result, bool printRes)
{
	int ret;

	ret = xmlClient->execute (methodName, in, result);
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


int
Client::doClient ()
{
	XmlRpcValue noArgs, oneArg, result;

	runXmlMethod ("system.listMethods", noArgs, result);

	oneArg[0] = "DeviceCount";
	runXmlMethod ("system.methodHelp", oneArg, result);

	runXmlMethod (R2X_VALUES_LIST, noArgs, result);

	XmlRpcValue threeArg;
	threeArg[0] = "C0";
	threeArg[1] = "scriptLen";
	threeArg[2] = 100;
	runXmlMethod (R2X_VALUE_SET, threeArg, result);

	threeArg[0] = "T0";
	threeArg[1] = "OFFS";
	threeArg[2] = "1 1";
	runXmlMethod (R2X_VALUE_SET, threeArg, result);

	threeArg[0] = "T0";
	threeArg[1] = "OBJ";
	threeArg[2] = "20 30";
	runXmlMethod (R2X_VALUE_SET, threeArg, result);

	runXmlMethod (R2X_DEVICES_LIST, noArgs, result);

	for (int i = 0; i < result.size (); i++)
	{
		XmlRpcValue resultValues;
		oneArg[0] = result[i];
		runXmlMethod (R2X_DEVICES_VALUES_LIST, oneArg, resultValues, false);
		std::cout << "listValues for device " << oneArg[0] << std::endl;
		for (int j = 0; j < resultValues.size(); j++)
		{
			std::cout << " " << resultValues[j]["name"]
				<< "=" << resultValues[j]["value"] << std::endl;
		}
	}

	oneArg[0] = "f";
	runXmlMethod (R2X_TARGETS_TYPE_LIST, oneArg, result);

	oneArg[0] = 2600;
	runXmlMethod (R2X_OBSERVATION_IMAGES_LIST, oneArg, result);

	oneArg[0] = 4;
	oneArg[1] = 2;
	runXmlMethod (R2X_TARGETS_INFO, oneArg, result);

	oneArg[0] = "log";
	oneArg[1] = "pas";
	return runXmlMethod (R2X_USER_LOGIN, oneArg, result);
}

int
Client::setVariable (const char *deviceName, const char *varName, const char *value)
{
	XmlRpcValue threeArg, result;

	threeArg[0] = deviceName;
	threeArg[1] = varName;
	threeArg[2] = value;

	return runXmlMethod (R2X_VALUE_SET, threeArg, result);
}


int
Client::schedTicketInfo (int ticketId)
{
	XmlRpcValue oneArg, result;
	oneArg = ticketId;

	return runXmlMethod (R2X_TICKET_INFO, oneArg, result);	
}


int
Client::getVariables ()
{
	int e = 0;
	// store results per device
	std::map <const char*, XmlRpcValue> results;
	char* a;
	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		const char* arg = (*iter);
		// look for dot..
		delete[] a;
		a = new char[strlen(arg)+1];
		strcpy (a, arg);
		char *dot = strchr (a, '.');
		if (dot == NULL)
		{
			e++;
			std::cerr << "Cannot parse " << arg << " - cannot find a dot separating devices" << std::endl;
			continue;
		}
		*dot = '\0';
		dot++;
		// get device - if it isn't present
		if (results.find (a) == results.end ())
		{
			XmlRpcValue oneArg, result;
			oneArg = a;
			int ret = runXmlMethod (R2X_DEVICES_VALUES_LIST, oneArg, result, false);
			if (ret)
			{
				e++;
				continue;
			}
			results[a] = result;
		}
		// find value among result..
		std::map <const char *, XmlRpcValue>::iterator riter = results.find (a);
		int j;
		for (j = 0; j < (*riter).second.size(); j++)
		{
			if ((*riter).second[j]["name"] == std::string (dot))
			{
				std::cout << a << "." << dot
					<< "=" << (*riter).second[j]["value"] << std::endl;
				break;
			}
		}
		if (j == (*riter).second.size ())
		{
			std::cerr << "Cannot find " << a << "." << dot << std::endl;
			e++;
		}
  	}
	delete[] a;
	return (e == 0) ? 0 : -1;
}

void
Client::usage ()
{
	std::cout << "  " << getAppName () << " -s <device name> <variable name> <value>" << std::endl
		<< " To set T0.OBJ to 10 20, run: " << std::endl
		<< "  " << getAppName () << " -s T0 OBJ \"10 20\"" << std::endl; 
}

int
Client::processOption (int in_opt)
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
		case 's':
			xmlOp = SET_VARIABLE;
			break;
		case OPT_SCHED_TICKET:
			schedTicket = atoi (optarg);
			xmlOp = SCHED_TICKET;
			break;
		case 'g':
			xmlOp = GET_VARIABLE;
			break;
		default:
			return Rts2CliApp::processOption (in_opt);
	}
	return 0;
}


int
Client::processArgs (const char *arg)
{
	if (!(xmlOp == SET_VARIABLE || xmlOp == GET_VARIABLE))
		return -1;
	args.push_back (arg);
	return 0;
}


int
Client::doProcessing ()
{
	switch (xmlOp)
	{
		case TEST:
			return doClient ();
		case SET_VARIABLE:
			if (args.size () != 3)
			{
				logStream (MESSAGE_ERROR) << "Invalid number of parameters - " << args.size () 
					<< ", expected 3." << sendLog;
				return -1;
			}
			return setVariable (args[0], args[1], args[2]);
		case SCHED_TICKET:
			return schedTicketInfo (schedTicket);
		case GET_VARIABLE:
			return getVariables ();
	}
	return -1;
}


int
Client::init ()
{
	int ret;
	ret = Rts2CliApp::init ();
	if (ret)
		return ret;
	XmlRpc::setVerbosity(xmlVerbosity);
	xmlClient = new XmlRpcClient (xmlHost, xmlPort);
	return 0;
}


Client::Client (int in_argc, char **in_argv): Rts2CliApp (in_argc, in_argv)
{
	xmlPort = 8889;
	xmlHost = "localhost";
	xmlVerbosity = 0;

	xmlOp = TEST;

	xmlClient = NULL;

	addOption (OPT_PORT, "port", 1, "port of XML-RPC server");
	addOption (OPT_HOST, "hostname", 1, "hostname of XML-RPC server");
	addOption ('v', NULL, 0, "verbosity (multiple -v to increase it)");
	addOption ('s', NULL, 0, "set variables specified by variable list.");
	addOption (OPT_SCHED_TICKET, "schedticket", 1, "print informations about scheduling ticket with given id");
	addOption ('g', NULL, 0, "get variables specified as arguments");
}


Client::~Client (void)
{
	delete xmlClient;
}


int main(int argc, char** argv)
{
	Client app (argc, argv);
	return app.run();
}
