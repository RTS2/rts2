/* 
 * JSON client.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "cliapp.h"

#include "iniparser.h"
#include "libnova_cpp.h"

#include <map>
#include <iomanip>

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

#define OPT_HOST                     OPT_LOCAL + 1
#define OPT_USERNAME                 OPT_LOCAL + 2
#define OPT_SCHED_TICKET             OPT_LOCAL + 3
#define OPT_TEST                     OPT_LOCAL + 4
#define OPT_MASTER_STATE             OPT_LOCAL + 5
#define OPT_TARGET_LIST              OPT_LOCAL + 6
#define OPT_QUIET                    OPT_LOCAL + 7

std::string username;
std::string password;

static void auth (SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying, gpointer data)
{
	if (retrying)
		return;
	soup_auth_authenticate (auth, username.c_str (), password.c_str ());
}

namespace rts2json
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
		const char *host;
		char *configFile;
		int verbosity;

		SoupSession *session;
		SoupLogger *logger;

		int schedTicket;
		enum {SET_VARIABLE, GET_STATE, GET_MASTER_STATE, SCHED_TICKET, COMMANDS, GET_VARIABLES_PRETTY, GET_VARIABLES, INC_VARIABLE, GET_TYPES, GET_MESSAGES, TARGET_LIST, HTTP_GET, TEST, NOOP} xmlOp;

		const char *masterStateQuery;

                bool getVariablesPrintNames;

		std::vector <const char *> args;

		/**
                 * Split variable name to 
		 */
		int splitDeviceVariable (const char *device, std::string &deviceName, std::string &varName);

		/**
		 * Run one JSON method. Prints out method execution
		 * processing according to verbosity level.
		 *
		 * @param url      Method Url 
		 * @param result    Call result.
		 * @param printRes     Prints out result.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int runJsonMethod (const std::string url, JsonParser *&result, bool printRes = true);

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
		int setVariable (const char *varName, const char *value);

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

		void printTargets (JsonNode *result);

		/**
		 * Resolve target(s) by name(s) and return them to user.
		 */
		int getTargets ();
};

}

using namespace rts2json;

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

int Client::runJsonMethod (const std::string url, JsonParser *&result, bool printRes)
{
	SoupMessage *msg;
	GError *error = NULL;

	std::ostringstream os;
	os << host << url;

	msg = soup_message_new (SOUP_METHOD_GET, os.str ().c_str ());

	soup_session_send_message (session, msg);

	if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code))
	{
		logStream (MESSAGE_ERROR) << "error calling '" << url << "': " << msg->status_code << " : " << msg->reason_phrase << sendLog;
		return -1;
	}

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
	{
		logStream (MESSAGE_ERROR) << "error calling '" << url << "': " << msg->status_code << " : " << msg->reason_phrase << sendLog;
		return -1;
	}

	result = json_parser_new ();

	json_parser_load_from_data (result, msg->response_body->data, strlen (msg->response_body->data), &error);
	if (error)
	{
		logStream (MESSAGE_ERROR) << "unable to parse " << url << sendLog;
		g_error_free (error);
		g_object_unref (msg);
		return -1;
	}

	g_object_unref (msg);

	return 0;
}

int Client::runHttpGet (char* url, bool printRes)
{
	SoupMessage *msg;

	msg = soup_message_new (SOUP_METHOD_GET, url);

	soup_session_send_message (session, msg);

	if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code))
	{
		logStream (MESSAGE_ERROR) << "error calling '" << url << "': " << msg->status_code << " : " << msg->reason_phrase << sendLog;
		return -1;
	}

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
	{
		logStream (MESSAGE_ERROR) << "error calling '" << url << "': " << msg->status_code << " : " << msg->reason_phrase << sendLog;
		return -1;
	}
	if (printRes)
	{
		std::cout << msg->response_body->data << std::endl;
	}

	g_object_unref (msg);

	return 0;
}

std::string uri_encode (const char *c)
{
	char *e_c = soup_uri_encode (c, NULL);
	std::string ret (e_c);
	free (e_c);
	return ret;
}

inline std::string uri_encode_s (const std::string s)
{
	return uri_encode (s.c_str ());
}

void printNode (JsonNode *node)
{
	switch (json_node_get_value_type (node))
	{
		case G_TYPE_INVALID:
			std::cout << "null";
			break;
		case G_TYPE_DOUBLE:
			std::cout << json_node_get_double (node);
			break;
		case G_TYPE_INT64:
			std::cout << json_node_get_int (node);
			break;
		case G_TYPE_BOOLEAN:
			std::cout << json_node_get_boolean (node);
			break;
		default:
			std::cout << json_node_get_string (node);
			break;
	}
}

void printArray (JsonArray *array, guint index, JsonNode *node, gpointer user_data)
{
	if (index > 0)
		std::cout << ",";
	printNode (node);
}

void printValue (JsonObject *object, const gchar *member_name, JsonNode *node, gpointer user_data);

void printNodeValue (JsonNode *node, bool pretty)
{
	switch (json_node_get_node_type (node))
	{
		case JSON_NODE_VALUE:
			printNode (node);
			break;
		case JSON_NODE_ARRAY:
			std::cout << "[";
			json_array_foreach_element (json_node_get_array (node), printArray, NULL);
			std::cout << "]";
			break;
		case JSON_NODE_OBJECT:
			if (pretty)
			{
				int count = 0;
				std::cout << "{";
				json_object_foreach_member (json_node_get_object (node), printValue, &count);
				std::cout << "}";
			}
			else
			{
				int count = -1;
				json_object_foreach_member (json_node_get_object (node), printValue, &count);
			}
			break;
		case JSON_NODE_NULL:
			std::cout << "NULL";
			break;
	}
}

void printValue (JsonObject *object, const gchar *member_name, JsonNode *node, gpointer user_data)
{
	if (user_data)
	{
		if (*((int *)user_data) < 0)
		{
			if (*((int *)user_data) < -1)
				std::cout << " ";

			(*((int *) user_data))--;
		}
		else
		{
			if (*((int *)user_data))
				std::cout << ",";
			(*((int *) user_data))++;
			std::cout << member_name << "=";
		}
	}

	printNodeValue (node, user_data != NULL);
}

void printValues (JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data)
{
	std::cout << " ";

	JsonNode *node = json_array_get_element (json_node_get_array (member_node), 1);

	printValue (object, member_name, node, user_data);

	std::cout << std::endl;
}

int Client::doTests ()
{
	JsonParser *ret;

	runJsonMethod (std::string ("/api/devices"), ret, true);

	JsonArray *aret = json_node_get_array (json_parser_get_root (ret));

	for (guint i = 0; i < json_array_get_length (aret); i++)
	{
		const gchar *dev = json_array_get_string_element (aret, i);

		JsonParser *ret1;

		std::ostringstream os;
		os << "/api/get?e=1&d=" << uri_encode (dev);
		runJsonMethod (os.str (), ret1, false);

		std::cout << "listValues for device " << dev << std::endl;

		JsonObject *values = json_object_get_object_member (json_node_get_object (json_parser_get_root (ret1)), "d");

		json_object_foreach_member (values, printValues, NULL);

		g_object_unref (ret1);
	}

	g_object_unref (ret);

/*	XmlRpcValue threeArg;
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
	return runXmlMethod (R2X_USER_LOGIN, oneArg, result); */
	return 0;
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
		int ret = runHttpGet ((char *) *iter);
		if (ret)
			return ret;
	}
	return 0;
}

int Client::testConnect ()
{
	JsonParser *res;
	runJsonMethod ("/api/devices", res, true);

	return 0;
}

int Client::executeCommands ()
{
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

		std::ostringstream os;

		os << "/api/cmd?d=" << uri_encode (c) << "&c=" << uri_encode (sep);

		JsonParser *res;
		int ret = runJsonMethod (os.str ().c_str (), res, false);

		if (ret)
			return ret;
	}
	return 0;
}

int Client::setVariable (const char *varName, const char *value)
{
	std::ostringstream os;

	std::string dnp, vn;
	if (splitDeviceVariable (varName, dnp, vn))
		return -1;

	os << "/api/set?d=" << uri_encode_s (dnp) << "&n=" << uri_encode_s (vn) << "&v=" << uri_encode (value);

	JsonParser *res;
	int ret = runJsonMethod (os.str ().c_str (), res);
	g_object_unref (res);

	return ret;
}

int Client::incVariable (const char *varName, const char *value)
{
	std::string dn, vn;
	if (splitDeviceVariable (varName, dn, vn))
		return -1;

	std::ostringstream os;
	
	os << "/api/inc?d=" << uri_encode_s (dn) << "&n=" << uri_encode_s (vn) << "&v=" << uri_encode (value);

	JsonParser *result;
	int ret = runJsonMethod (os.str ().c_str (), result);
	g_object_unref (result);

	return ret;
}

int Client::getState (const char *devName)
{
	std::ostringstream os;
	os << "/api/get?d=" << uri_encode (devName);

	JsonParser *result = NULL;
	int ret = runJsonMethod (os.str ().c_str (), result);
	if (ret)
	{
		g_object_unref (result);
		return ret;
	}
	std::cout << devName << " " << json_object_get_int_member (json_node_get_object (json_parser_get_root (result)), "state") << std::endl;
	return ret; 
}

int Client::getMasterState ()
{
	JsonParser *parser;
	int ret = runJsonMethod ("/api/get?d=centrald", parser);
	if (ret)
		return ret;
	bool res;

	uint64_t state = json_object_get_int_member (json_node_get_object (json_parser_get_root (parser)), "state");
	if (masterStateQuery)
	{
		if (!strcasecmp (masterStateQuery, "on"))
			res = (state & SERVERD_STATUS_MASK) != SERVERD_HARD_OFF && (state & SERVERD_STATUS_MASK) != SERVERD_SOFT_OFF && !(state & SERVERD_STANDBY_MASK);
		else if (!strcasecmp (masterStateQuery, "standby"))
			res = state & SERVERD_STANDBY_MASK;
		else if (!strcasecmp (masterStateQuery, "off"))
			res = (state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF || (state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF;
		else if (!strcasecmp (masterStateQuery, "rnight"))
			res = (state & SERVERD_STATUS_MASK) == SERVERD_NIGHT && !(state & SERVERD_STANDBY_MASK);
		else
		{
			std::cerr << "Invalid state parameter " << masterStateQuery << std::endl;
			return -1;
		}

		std::cout << res << std::endl;
	}
	else
	{
		std::cout << state << std::endl;
	}
	return 0;
}

int Client::schedTicketInfo (int ticketId)
{
/*	XmlRpcValue oneArg, result;
	oneArg = ticketId;

	return runXmlMethod (R2X_TICKET_INFO, oneArg, result);	 */
	return 0;
}

int Client::getVariables (bool pretty)
{
	int e = 0;
	// store results per device
	std::map <const char*, JsonParser *> results;
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
			if (verbosity >= 0)
				std::cerr << "Cannot parse " << arg << " - cannot find a dot separating devices" << std::endl;
			continue;
		}
		*dot = '\0';
		dot++;
		// get device - if it isn't present
		if (results.find (a) == results.end ())
		{
			std::ostringstream os;
			os << "/api/get?d=" << uri_encode (a);
			int ret = runJsonMethod (os.str ().c_str (), results[a]);
			if (ret)
			{
				e++;
				continue;
			}
		}
		// find value among result..
		std::map <const char*, JsonParser*>::iterator riter = results.find (a);

		JsonNode *n = json_object_get_member (json_object_get_object_member (json_node_get_object (json_parser_get_root (riter->second)), "d"), dot);
		if (n)
		{
			if (getVariablesPrintNames)
			{
				std::cout << a << "_" << dot << "=";
			}

			printNodeValue (json_object_get_member (json_object_get_object_member (json_node_get_object (json_parser_get_root (riter->second)), "d"), dot), false);
			std::cout << std::endl;
		}
		else
		{
			if (verbosity >= 0)
				std::cerr << "Cannot find " << a << "." << dot << std::endl;
			e++;
		}
  	}

	for (std::map <const char*, JsonParser*>::iterator iter = results.begin (); iter != results.end (); iter++)
	{
		g_object_unref (iter->second);
	}
	delete[] a;
	return (e == 0) ? 0 : -1;
}

int Client::getTypes ()
{
/*	int e = 0;
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
	return (e == 0) ? 0 : -1; */
	return 0;
}

void printMessage (JsonArray *array, guint index, JsonNode *node, gpointer user_data)
{
	JsonArray *arr = json_node_get_array (node);
	std::cout << std::setw (5) << std::right << (index + 1) << ": "
		<< LibnovaDateDouble (json_array_get_double_element (arr, 0)) << " "
		<< json_array_get_string_element (arr, 1) << " "
		<< json_array_get_int_element (arr, 2) << " "
		<< json_array_get_string_element (arr, 3)
		<< std::endl;
}

int Client::getMessages ()
{
	JsonParser *result = NULL;

	int ret = runJsonMethod ("/api/msgqueue", result);
	if (ret)
	{
		g_object_unref (result);
		return ret;
	}

	json_array_foreach_element (json_object_get_array_member (json_node_get_object (json_parser_get_root (result)), "d"), printMessage, NULL);

	g_object_unref (result);
	return ret;
}

void Client::printTargets (JsonNode *results)
{
//	for (int i=0; i < results.size (); i++)
//	{
//		std::cout << results[i]["id"] << " " << results[i]["type"] << " " << results[i]["name"] << " " << LibnovaRa (results[i]["ra"]) << " " << LibnovaDec (results[i]["dec"]) << std::endl;
//	}
}

int Client::getTargets ()
{
/*	int tot = 0;

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
		if (verbosity >= 0)
			std::cerr << "cannot find any targets" << std::endl;
		return -1;
	} */

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
		case OPT_HOST:
			host = optarg;
			break;
		case OPT_USERNAME:
			username = optarg;
			break;
		case OPT_CONFIG:
			configFile = new char[strlen(optarg) + 1];
			strcpy (configFile, optarg);
			break;
		case OPT_QUIET:
			verbosity = -1;
			break;
		case 'v':
			verbosity++;
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
			return setVariable (args[0], args[1]);
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
	if (username.length () == 0)
	{
		ret = config.getString ("json", "username", username);
		if ((ret || username.length()) == 0 && xmlOp != HTTP_GET)
		{
			if (verbosity >= 0)
				std::cerr << "You don't specify authorization string in XML-RPC config file, nor on command line." << std::endl;
			return -1;
		}
		config.getString ("json", "password", password);
	}
	else // authorization from command line..
	{
		ret = askForPassword ("password", password);
		if (ret)
			return -1;
	}

	g_type_init ();

	session = soup_session_sync_new_with_options (
		SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
		SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
		SOUP_SESSION_USER_AGENT, "rts2 ",
		NULL);

	if (verbosity > 1)
	{
		logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
		soup_logger_attach (logger, session);
	}

	if (username.length () > 0)
		g_signal_connect (session, "authenticate", G_CALLBACK (auth), NULL);

	std::cout << std::fixed;

	return 0;
}


Client::Client (int in_argc, char **in_argv): rts2core::CliApp (in_argc, in_argv)
{
	host = "http://localhost:8889";
	configFile = NULL;
	verbosity = 0;

	session = NULL;
	logger = NULL;

	xmlOp = NOOP;

        getVariablesPrintNames = true;

	masterStateQuery = NULL;

	addOption ('v', NULL, 0, "verbosity (multiple -v to increase it)");
	addOption (OPT_QUIET, "quiet", 0, "don't report errors on stderr");
	addOption (OPT_CONFIG, "config", 1, "configuration file (default to ~/.rts2)");
	addOption (OPT_USERNAME, "user", 1, "username for XML-RPC server authorization");
	addOption (OPT_HOST, "hostname", 1, "hostname of XML-RPC server");
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
	g_object_unref (session);
}

int main(int argc, char** argv)
{
	Client app (argc, argv);
	return app.run();
}
