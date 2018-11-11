/* 
 * XML-RPC client.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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
#include "cliapp.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <iomanip>

#include "r2x.h"
#include "iniparser.h"
#include "libnova_cpp.h"

using namespace XmlRpc;

#define OPT_HOST                     OPT_LOCAL + 1
#define OPT_USERNAME                 OPT_LOCAL + 2
#define OPT_SCHED_TICKET             OPT_LOCAL + 3
#define OPT_TEST                     OPT_LOCAL + 4
#define OPT_MASTER_STATE             OPT_LOCAL + 5
#define OPT_TARGET_LIST              OPT_LOCAL + 6
#define OPT_QUIET                    OPT_LOCAL + 7

namespace rts2xmlrpc
{

/**
 * Class for testing XML-RPC.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Client: public rts2core::CliApp
{
	public:
		Client (int argc, char **argv);
		virtual ~Client (void);

	protected:
		virtual void usage ();

		virtual int processOption (int opt);
		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();

	private:
		int xmlPort;
		const char *xmlHost;
		const char *xmlUsername;
		std::string xmlAuthorization;
		char *configFile;
		int xmlVerbosity;

		int schedTicket;
		enum {SET_VARIABLE, GET_STATE, GET_MASTER_STATE, SCHED_TICKET, COMMANDS, GET_VARIABLES_PRETTY, GET_VARIABLES, INC_VARIABLE, GET_TYPES, GET_MESSAGES, TARGET_LIST, HTTP_GET, TEST, NOOP} xmlOp;

		const char *masterStateQuery;

                bool getVariablesPrintNames;

		std::vector <const char *> args;

		XmlRpcClient* xmlClient;
		XmlRpcClient* httpClient;

		/**
                 * Split variable name to 
		 */
		int splitDeviceVariable (const char *device, std::string &deviceName, std::string &varName);

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
		 * Run HTTP GET request.
		 */
		int runHttpGet (char* path, bool printRes = true);

		/**
		 * Run test methods.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int doTests ();

		int doHttpGet ();

		/**
		 * Test that XML-RPC daemon is running.
		 */
		int testConnect ();

		/**
		 * Execute command(s) on device(s)
		 */
		int executeCommands ();

		/**
		 * Set XML-RPC variable.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setVariables (const char *varName, const char *value);

		/**
		 * Return device state.
		 */
		int getState (const char *devName);

		int getMasterState ();

		/**
		 * Increase XML-RPC variable.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int incVariable (const char *varName, const char *value);

		/**
		 * Prints informations about given ticket.
		 *
		 * @param ticketId ID of ticket to print.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int schedTicketInfo (int ticketId);

		/**
		 * Return variables. Variables names are in args.
		 */
		int getVariables (bool pretty);

		/**
		 * Return device types. Device names are in args.
		 */
		int getTypes ();

		/**
		 * Retrieve messages from message buffer.
		 */
		int getMessages ();

		void printTargets (XmlRpcValue &result);

		/**
		 * Resolve target(s) by name(s) and return them to user.
		 */
		int getTargets ();
};

}

using namespace rts2xmlrpc;

int Client::splitDeviceVariable (const char *device, std::string &deviceName, std::string &varName)
{
	std::string full(device);
	std::size_t dot = full.find ('.');
	if (dot == std::string::npos)
		return -1;
	deviceName = full.substr (0, dot);
	varName = full.substr (dot + 1);
	return 0;
}

int Client::runXmlMethod (const char* methodName, XmlRpcValue &in, XmlRpcValue &result, __attribute__ ((unused)) bool printRes)
{
	int ret;

	ret = xmlClient->execute (methodName, in, result);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "error calling '" << methodName << "' with arguments " << ((std::string) in) << sendLog;
		return -1;
	}
	return 0;
}

int Client::runHttpGet (char* path, bool printRes)
{
	int ret;

	char* reply = NULL;
	int reply_length = 0;

	const char *_uri;

	httpClient = new XmlRpcClient (path, &_uri);

	ret = httpClient->executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error requesting " << path << sendLog;
		delete httpClient;
		return -1;
	}
	if (printRes)
	{
		std::cout << reply << std::endl;
	}
	free(reply);
	delete httpClient;
	return 0;
}

int Client::doTests ()
{
	XmlRpcValue noArgs, oneArg, result;

	runXmlMethod ("system.listMethods", noArgs, result);

	oneArg[0] = "DeviceCount";
	runXmlMethod ("system.methodHelp", oneArg, result);

	XmlRpcValue twoArg;

	try
	{
		runXmlMethod (R2X_DEVICES_LIST, oneArg, result);

	} catch (XmlRpcException &e)
	{
		if (xmlVerbosity >= 0)
			std::cerr << "Cannot receive list of devices: " << e.getMessage () << std::endl;
	}

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
	threeArg[1] = "ORI";
	threeArg[2] = "20 30";
	runXmlMethod (R2X_VALUE_SET, threeArg, result);

	runXmlMethod (R2X_TARGETS_LIST, noArgs, result);

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

int Client::doHttpGet ()
{
	if (args.size () == 0)
	{
		logStream (MESSAGE_ERROR) << "You must specify at least one path as argument." << sendLog;
		return -1;
	}

	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		try
		{
			int ret = runHttpGet ((char *) *iter);
			if (ret)
				return ret;
		}
		catch (XmlRpcException &e)
		{
			if (xmlVerbosity >= 0)
				std::cerr << "Cannot retrieve document " << *iter << std::endl;
			return -1;
		}
	}
	return 0;
}

int Client::testConnect ()
{
	XmlRpcValue nullArg, result;

	try
	{
		runXmlMethod (R2X_DEVICES_LIST, nullArg, result);
	} catch (XmlRpcException &e)
	{
		if (xmlVerbosity >= 0)
			std::cerr << "Cannot receive list of devices: " << e.getMessage () << std::endl;
		return -1;
	}

	return 0;
}

int Client::executeCommands ()
{
	XmlRpcValue twoArg, result;

	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		char c[strlen(*iter) + 1];
		strcpy (c, *iter);
		char *sep = strchr (c, '.');
		if (sep == NULL)
		{
			logStream (MESSAGE_ERROR) << "cannot find . separating device name and command in " << (*iter) << "." << sendLog;
			return -1;
		}
		*sep = '\0';
		sep++;

		twoArg[0] = c;
		twoArg[1] = sep;

		int ret = runXmlMethod (R2X_DEVICE_COMMAND, twoArg, result, false);
		if (ret)
			return ret;
	}
	return 0;
}

int Client::setVariables (const char *varName, const char *value)
{
	XmlRpcValue threeArg, result;

	std::string dnp;
	if (splitDeviceVariable (varName, dnp, threeArg[1]))
		return -1;
	threeArg[2] = value;

	// device name is number - run R2X_VALUE_BY_TYPE_SET
	char *endp;
	int dt = strtol (dnp.c_str (), &endp, 10);
	if (*endp == '\0' && dnp.length () > 0)
	{
		threeArg[0] = dt;
		return runXmlMethod (R2X_VALUE_BY_TYPE_SET, threeArg, result);
	}
	threeArg[0] = dnp;
	return runXmlMethod (R2X_VALUE_SET, threeArg, result);
}

int Client::incVariable (const char *varName, const char *value)
{
	XmlRpcValue threeArg, result;
	if (splitDeviceVariable (varName, threeArg[0], threeArg[1]))
		return -1;
	threeArg[2] = value;

	return runXmlMethod (R2X_VALUE_SET, threeArg, result);
}

int Client::getState (const char *devName)
{
	XmlRpcValue oneArg, result;
	oneArg[0] = devName;

	int ret = runXmlMethod (R2X_DEVICE_STATE, oneArg, result, false);
	if (ret)
		return ret;
	std::cout << devName << " " << result[1] << " " << result[0] << std::endl;
	return ret;
}

int Client::getMasterState ()
{
	XmlRpcValue oneArg, result;
	int ret;

	if (masterStateQuery)
	{
		oneArg[0] = masterStateQuery;
		ret = runXmlMethod (R2X_MASTER_STATE_IS, oneArg, result, false);
	}
	else
	{
		ret = runXmlMethod (R2X_MASTER_STATE, oneArg, result, false);
	}
	if (ret)
		return ret;
	if (masterStateQuery)
		std::cout << result << std::endl;
	else
		std::cout << (unsigned int) ((int) result) << std::endl;
	return ret;
}

int Client::schedTicketInfo (int ticketId)
{
	XmlRpcValue oneArg, result;
	oneArg = ticketId;

	return runXmlMethod (R2X_TICKET_INFO, oneArg, result);	
}

int Client::getVariables (bool pretty)
{
	int e = 0;
	// store results per device
	std::map <const char*, XmlRpcValue> results;
	char* a = NULL;
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
			if (xmlVerbosity >= 0)
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
			int ret = runXmlMethod (pretty ? R2X_DEVICES_VALUES_PRETTYLIST : R2X_DEVICES_VALUES_LIST, oneArg, result, false);
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
				bool printEndAmp = false;
				std::ostringstream osv;
                                if (getVariablesPrintNames)
				{
				        std::cout << a << "_" << dot << "=";
					osv << (*riter).second[j]["value"];
					std::string val = osv.str ();
					if (val.find (' ') != std::string::npos)
					{
						std::cout << '"';
						printEndAmp = true;
					}
				}
				std::cout << std::fixed << (*riter).second[j]["value"];
				if (printEndAmp)
					std::cout << '"';
				std::cout << std::endl;
				break;
			}
		}
		if (j == (*riter).second.size ())
		{
			if (xmlVerbosity >= 0)
				std::cerr << "Cannot find " << a << "." << dot << std::endl;
			e++;
		}
  	}
	delete[] a;
	return (e == 0) ? 0 : -1;
}

int Client::getTypes ()
{
	int e = 0;
	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		const char* arg = (*iter);
		XmlRpcValue oneArg, result;
		oneArg = arg;
		int ret = runXmlMethod (R2X_DEVICE_TYPE, oneArg, result, false);
		if (ret)
		{
			e++;
			continue;
		}
		std::cout << arg << " TYPE " << result << std::endl;
  	}
	return (e == 0) ? 0 : -1;
}

int Client::getMessages ()
{
	XmlRpcValue nullArg, result;

	int ret = runXmlMethod (R2X_MESSAGES_GET, nullArg, result, false);

	std::cout << "size " << result.size () << std::endl;

	for (int i = 0; i < result.size (); i++)
	{
		std::cout << std::setw (5) << std::right << (i + 1) << ": "
			<< result[i]["timestamp"] << " "
			<< result[i]["origin"] << " "
			<< result[i]["type"] << " "
			<< result[i]["message"]
			<< std::endl;
	}
	
	return ret;
}

void Client::printTargets (XmlRpcValue &results)
{
	for (int i=0; i < results.size (); i++)
	{
		std::cout << results[i]["id"] << " " << results[i]["type"] << " " << results[i]["name"] << " " << LibnovaRa (results[i]["ra"]) << " " << LibnovaDec (results[i]["dec"]) << std::endl;
	}
}

int Client::getTargets ()
{
	XmlRpcValue tarNames, result;

	int tot = 0;

	if (args.empty ())
	{
		int ret = runXmlMethod (R2X_TARGETS_LIST, tarNames, result, false);
		if (ret)
			return ret;
		tot += result.size ();
		printTargets (result);
	}

	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		tarNames[0] = *iter;
		int ret = runXmlMethod (R2X_TARGETS_LIST, tarNames, result, false);

		if (ret)
			return ret;
		tot += result.size ();
		printTargets (result);
	}
	if (tot == 0)
	{
		if (xmlVerbosity >= 0)
			std::cerr << "cannot find any targets" << std::endl;
		return -1;
	}

	return 0;
}

void Client::usage ()
{
	std::cout << "In most cases you must specify username and password. You can specify them either in configuration file (default to ~/.rts2) or on command line. Please consult manual pages for details." << std::endl << std::endl
		<< "  " << getAppName () << " -s <device name>.<variable name> <value>" << std::endl
		<< " To set T0.OFFS to -10' 20\" (-10 arcmin in RA, 20 arcsec in DEC), run: " << std::endl
		<< "  " << getAppName () << " -s T0.OFFS -- \"-10' 20\\\"\"" << std::endl
		<< " So to run random pointing, run: " << std::endl
		<< "  " << getAppName () << " -s T0.ORI \"`rts2-telmodeltest -r`\"" << std::endl
		<< "  " << getAppName () << " -c <device name>.<command>" << std::endl
		<< " So to run park command on T0 device:" << std::endl
		<< "  " << getAppName () << " -c T0.park" << std::endl
		<< "  " << getAppName () << " -i <device name>.<variable name> <value>" << std::endl
		<< " So to increase value T0.OFFS by 0.1 in RA and DEC, run: " << std::endl
		<< "  " << getAppName () << " -i T0.OFFS \"0.1 0.1\"" << std::endl
		<< " To get master state as number: " << std::endl
		<< "  " << getAppName () << " --master-state" << std::endl
		<< " To check if state is on (other posible arguments are standby or off):" << std::endl
		<< "  " << getAppName () << " --master-state on" << std::endl
		<< " Default action, e.g. running " << getAppName () << " without any parameters, will result in printout if the XML-RPC server is accessible" << std::endl;
}

int Client::processOption (int opt)
{
	int old_op = xmlOp;
	switch (opt)
	{
		case OPT_PORT:
			xmlPort = atoi (optarg);
			break;
		case OPT_HOST:
			xmlHost = optarg;
			break;
		case OPT_USERNAME:
			xmlUsername = optarg;
			break;
		case OPT_CONFIG:
			configFile = new char[strlen(optarg) + 1];
			strcpy (configFile, optarg);
			break;
		case OPT_QUIET:
			xmlVerbosity = -1;
			break;
		case 'v':
			xmlVerbosity++;
			break;
		case 'c':
			xmlOp = COMMANDS;
			break;
		case 's':
			xmlOp = SET_VARIABLE;
			break;
		case 'i':
			xmlOp = INC_VARIABLE;
			break;
		case 'S':
			xmlOp = GET_STATE;
			break;
		case OPT_SCHED_TICKET:
			schedTicket = atoi (optarg);
			xmlOp = SCHED_TICKET;
			break;
		case 'g':
			xmlOp = GET_VARIABLES;
                        getVariablesPrintNames = true;
			break;
		case 'G':
			xmlOp = GET_VARIABLES;
                        getVariablesPrintNames = false;
			break;
		case 'p':
			xmlOp = GET_VARIABLES_PRETTY;
                        getVariablesPrintNames = true;
			break;
		case 'P':
			xmlOp = GET_VARIABLES_PRETTY;
                        getVariablesPrintNames = false;
			break;
		case 't':
			xmlOp = GET_TYPES;
			break;
		case OPT_TEST:
			xmlOp = TEST;
			break;
		case OPT_MASTER_STATE:
			xmlOp = GET_MASTER_STATE;
			break;
		case OPT_TARGET_LIST:
			xmlOp = TARGET_LIST;
			break;
		case 'm':
			xmlOp = GET_MESSAGES;
			break;
		case 'u':
			xmlOp = HTTP_GET;
			break;
		default:
			return rts2core::CliApp::processOption (opt);
	}
	if (xmlOp != old_op && old_op != NOOP)
	{
		logStream (MESSAGE_ERROR) << "Cannot provide more then a single action command, option " << ((char) opt) << " cannot be processed" << sendLog;
		return -1;
	}
	return 0;
}

int Client::processArgs (const char *arg)
{
	if (xmlOp == GET_MASTER_STATE && masterStateQuery == NULL)
	{
		masterStateQuery = arg;
		return 0;
	}
	if (!(xmlOp == COMMANDS || xmlOp == SET_VARIABLE || xmlOp == GET_VARIABLES || xmlOp == GET_VARIABLES_PRETTY || xmlOp == INC_VARIABLE || xmlOp == GET_STATE || xmlOp == GET_TYPES || xmlOp == HTTP_GET || xmlOp == TARGET_LIST))
		return -1;
	args.push_back (arg);
	return 0;
}

int Client::doProcessing ()
{
	switch (xmlOp)
	{
		case COMMANDS:
			return executeCommands ();
		case SET_VARIABLE:
			if (args.size () != 2)
			{
				logStream (MESSAGE_ERROR) << "Invalid number of parameters - " << args.size () 
					<< ", expected 2." << sendLog;
				return -1;
			}
			return setVariables (args[0], args[1]);
		case INC_VARIABLE:
			if (args.size () == 0)
			{
				logStream (MESSAGE_ERROR) << "Invalid number of parameters - " << args.size () << ", expected 2." << sendLog;
				return -1;
			}
			return incVariable (args[0], args[1]);
		case GET_STATE:
			if (args.size () == 0)
			{
				logStream (MESSAGE_ERROR) << "Missing argument (device names)." << sendLog;
				return -1;
			}
			for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
				getState (*iter);
			return 0;
		case GET_MASTER_STATE:
			return getMasterState ();
		case SCHED_TICKET:
			return schedTicketInfo (schedTicket);
		case GET_VARIABLES:
			return getVariables (false);
		case GET_VARIABLES_PRETTY:
			return getVariables (true);
		case GET_TYPES:
			return getTypes ();
		case GET_MESSAGES:
			return getMessages ();
		case TARGET_LIST:
			return getTargets ();
		case TEST:
			return doTests ();
		case HTTP_GET:
			return doHttpGet ();
		case NOOP:
			return testConnect ();
	}
	return -1;
}

int Client::init ()
{
	int ret;
	ret = rts2core::CliApp::init ();
	if (ret)
		return ret;

	if (configFile == NULL)
	{
		char *homeDir = getenv ("HOME");
		int homeLen = strlen (homeDir);
		configFile = new char[homeLen + 7];
		strcpy (configFile, homeDir);
		strcpy (configFile + homeLen, "/.rts2");
	}

	rts2core::IniParser config = rts2core::IniParser ();
	config.loadFile (configFile);
	if (xmlUsername == NULL)
	{
		ret = config.getString ("xmlrpc", "authorization", xmlAuthorization);
		if ((ret || xmlAuthorization.length()) == 0 && xmlOp != HTTP_GET)
		{
			if (xmlVerbosity >= 0)
				std::cerr << "You don't specify authorization string in XML-RPC config file, nor on command line." << std::endl;
			return -1;
		}
	}
	else // authorization from command line..
	{
		std::string passw;
		ret = askForPassword ("password", passw);
		if (ret)
			return -1;
		xmlAuthorization = std::string (xmlUsername);
		xmlAuthorization += ":";
		xmlAuthorization += passw;
	}
	XmlRpc::setVerbosity(xmlVerbosity);
	xmlClient = new XmlRpcClient (xmlHost, xmlPort, xmlAuthorization.c_str());
	return 0;
}


Client::Client (int in_argc, char **in_argv): rts2core::CliApp (in_argc, in_argv)
{
	xmlPort = 8889;
	xmlHost = "localhost";
	xmlUsername = NULL;
	xmlAuthorization = std::string ("");
	configFile = NULL;
	xmlVerbosity = 0;

	xmlOp = NOOP;

        getVariablesPrintNames = true;

	xmlClient = NULL;

	masterStateQuery = NULL;

	addOption ('v', NULL, 0, "verbosity (multiple -v to increase it)");
	addOption (OPT_QUIET, "quiet", 0, "don't report errors on stderr");
	addOption (OPT_CONFIG, "config", 1, "configuration file (default to ~/.rts2)");
	addOption (OPT_USERNAME, "user", 1, "username for XML-RPC server authorization");
	addOption (OPT_HOST, "hostname", 1, "hostname of XML-RPC server");
	addOption (OPT_PORT, "port", 1, "port of XML-RPC server");
	addOption (OPT_TEST, "test", 0, "perform various tests");
	addOption ('u', NULL, 0, "retrieve given path(s) from the server (through HTTP GET request)");
	addOption ('t', NULL, 0, "get device(s) type");
	addOption ('m', NULL, 0, "retrieve messages from XML-RPCd message buffer");
	addOption (OPT_MASTER_STATE, "master-state", 0, "retrieve master state (as single value) or ask if the system is in on/standby/off/rnight)");
	addOption (OPT_SCHED_TICKET, "schedticket", 1, "print informations about scheduling ticket with given id");
	addOption (OPT_TARGET_LIST, "targets", 0, "list all targets (or those whose names matches the arguments)");
	addOption ('c', NULL, 0, "execute given command(s)");
	addOption ('S', NULL, 0, "get state of device(s) specified as argument(s)");
	addOption ('i', NULL, 0, "increment variable(s) specified as argument(s)");
	addOption ('s', NULL, 0, "set variables specified by variable list");
	addOption ('P', NULL, 0, "pretty print value(s) specified as argument(s), separated with new line");
	addOption ('p', NULL, 0, "pretty print value(s) specified as argument(s)");
        addOption ('G', NULL, 0, "get value(s) specified as arguments, print them separated with new line");
	addOption ('g', NULL, 0, "get value(s) specified as arguments");
}

Client::~Client (void)
{
	delete[] configFile;
	delete xmlClient;
}

int main(int argc, char** argv)
{
	Client app (argc, argv);
	return app.run();
}
