/* 
 * API access for RTS2.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

/**
 * @page JSON JSON (Java Script Object Notation) API calls
 *
 * The <a href='http://www.json.org'>Java Script Object Notation</a> is a protocol to transmit
 * structured information on text file (or mostly make them available as HTTP pages). 
 * The <a href='http://rts2.org>RTS2</a> API exposes various, both RTS2 devices manipulation, through
 * various calls. The calls are simply URLs, with arguments to parametrize those calls. When 
 * the URL is called, retrieved text is in JSON format. This return text containts all data need to 
 *
 * @section Involved components
 *
 * For your installation to have available JSON API, you must have \ref rts2-xmlrpcd up and running.
 * The following diagram gets an overview how both database and device calls are handled.
 *
 * @dot
digraph "JSON API calls handling" {
  "Client" [shape=box];
  "HTTP/JSON library" [shape=polygon];
  "rts2-xmlrpcd" [shape=box];
  "RTS2 device" [shape=box];
  "SQL database" [shape=box];

  "Client" -> "HTTP/JSON library" [label="function call"];
  "HTTP/JSON library" -> "rts2-xmlrpcd" [label="HTTP GET request"];
  "rts2-xmlrpcd" -> "RTS2 device" [label="RTS2 protocol request call"];
  "rts2-xmlrpcd" -> "SQL database" [label="SQL call"];

  "rts2-xmlrpcd" -> "HTTP/JSON library" [label="JSON document with returned data"];
  "HTTP/JSON library" -> "Client" [label="function return"];
}
 * @enddot
 * 
 * @section JSON_API_errors Error handling
 *
 * Calls to existing API points with invalid arguments returns a valid JSON
 * document with error decription. This happens e.g. if one call \ref JSON_device_set
 * with devices not present in the system.
 *
 * Calls to non-existent pages returns proper HTTP error codes.
 *
 * @section JSON_API_calls JSON API calls
 *
 * The calls were grouped to two categories, which are linked below:
 *
 *  - @ref JSON_device
 *  - @ref JSON_sql
 *
 * @section JSON_description Description of API call entries
 *
 * Let's start with an example demonstrating how a single call documentation might look:
 *
 * @section JSON_desc_example example
 *
 * Do nothigh, usefull only to test the JSON-API infrastructure.
 *
 * @subsection Example
 *
 * http://localhost:8889/api/example?doit=f&expand=%N/%u
 *
 * @subsection Parameters
 *
 *  - <b>doit</b>  If operation should be performed. Required.
 *  - <b>expand</b>  <i>Expansion string for a file created at random path.</i>
 *
 * @subsection Return
 *
 * So the documentation consists of name of the call, Example in the section
 * above. Name is followed by description of the call, its purpose and anything
 * related to the call.
 *
 * Then follows subsection. The one named Example containts example call, assuming that
 * server is running on localhost, on standard port 8889, and you don't need username and login
 * to access @ref rts2-xmlrpcd server from the localhost.
 *
 * Parameters are described in special section. Parameter name is <b>bold</b>. Parameters
 * that are optional has description written in <i>italic</i>, while required parameters
 * has description in plain text.
 *
 * Last is the return section, which describes format and content of (usually JSON) data
 * returned from the call.
 *
 * @page JSON_device JSON devices API
 *
 * Please see @ref JSON_description for description of this documentation.
 *
 * <hr/>
 *
 * @section JSON_device_expose expose
 *
 * Start exposure on given camera. Queue exposure if exposure is already in progress.
 * Most of the image parameters (exposure length, dark vs. light image) can be set through
 * variables present in the camera device (exposure, shutter,..).
 *
 * @subsection Example
 *
 * http://localhost:8889/api/expose?ccd=C0&fe=%N_%05u.fits
 *
 * Please note that example above expect that % and / characters will be
 * properly URI encoded before passing as paremeters to GET call.
 *
 * @subsection Parameters
 *  - <b>ccd</b>  Name of CCD device. Required.
 *  - <i><b>fe</b>  File expansion string. Can include expansion characters. Default to filename expansion string provided in rts2.ini configuration file.
          Please See man rts2.ini and man rts2 for details about configuration (xmlrpcd/images_name) and expansion characters.</i>
 *
 * @subsection Return
 *
 * Camera values in JSON format. Please see @ref JSON_device_get for details.
 *
 * <hr/>
 *
 * @section JSON_device_get get
 *
 * Retrieve values from given device.
 *
 * @subsection Example
 *
 * http://localhost:8889/api/get?d=C0&e=1&from=10000000
 *
 * @subsection Parameters
 *
 * - <b>d</b> Device name. Can be <i>centrald</i> to retrieve data from rts2-centrald.
 * - <i><b>e</b> Extended format. If set to 1, returned structure will containt with values some meta-informations. Default to 0.</i>
 * - <i><b>from</b> Updates will be taken from this time. If not specified, all values will be send back. If specified, only values updated 
 *         from the given time (in ctime, e.g. seconds from 1/1/1970) will be send.</i>
 *
 * @subsection Return
 *
 * The returned structure is a complex JSON structure, with nested hashes and arrays. Format of 
 * the returned data depends on <b>e</b> parameter.
 * Simple format, e.g. when e parameter is 0, is following.
 *
 * - <b>d</b>:{ <i>data values</i>
 *   - {<b>value name</b>:<b>value</b>,...}
 * - }
 * - <b>minmax</b>:{ <i>list of variables with minimal/maximal allowed value</i>
 *   - {<b>value name</b>:[<b>min</b>,<b>max</b>],...}
 * - }
 * - <b>idle</b>:<b>0|1</b> <i>idle state. 1 if device is idle</i>
 * - <b>stat</b>:<b>device stat</b> <i>full device state</i>
 * - <b>f</b>:<b>time</b> <i>actual time. Can be used in next get query as from parameter</i>
 *
 * When extended format is requested with <b>e=1</b>, then instead of returning
 * values in d, array with those members is returned:
 *  - [
 *  - <b>flags</b>, <i>value flags, describing its type,..</i>
 *  - <b>value</b>,
 *  - <b>isError</b>, <i>1 if value has signalled error</i>
 *  - <b>isWarning</b>, <i>1 if value has signalled warning</i>
 *  - <b>description</b> <i>short description of the variable</i>
 *  - ]
 *
 * <hr/>
 *
 * @section JSON_device_set set
 *
 * Set variable on server.
 *
 * @section JSON_device_script runscript
 *
 * Run script on device.
 *
 * @subsection Example
 *
 * http://localhost:8889/api/runscript?d=C0&s=E 20&kill=1&fe=%N_%05u.fits
 *
 * @subsection Parameters
 *
 *  - <b>d</b> Device name. Device must be CCD/Camera.
 *  - <b>s</b> Script. Please bear in mind that you should URI encode any special characters in the script. Please see <b>man rts2.script</b> for details.
 *  - <i><b>kill</b> If 1, current script will be killed. Default to 0, which means finish current action on device, and then start new script.</i>
 *  - <i><b>fe</b>  File expansion string. Can include expansion characters. Default to filename expansion string provided in rts2.ini configuration file.
          Please See man rts2.ini and man rts2 for details about configuration (xmlrpcd/images_name) and expansion characters.</i>
 *
 * @subsection Return
 * 
 * @page JSON_sql JSON database API
 *
 * @section targets List available targets
 *
 */

#include "xmlrpcd.h"

#include "../../lib/rts2db/constraints.h"
#include "../../lib/rts2db/planset.h"
#include "../../lib/rts2script/script.h"

#ifdef HAVE_PGSQL
#include "../../lib/rts2db/labellist.h"
#include "../../lib/rts2db/simbadtarget.h"
#include "../../lib/rts2db/messagedb.h"
#endif

using namespace rts2xmlrpc;

AsyncAPI::AsyncAPI (API *_req, rts2core::Connection *_conn, XmlRpcServerConnection *_source, bool _ext):Rts2Object ()
{
	// that's legal - requests are statically allocated and will cease exists with the end of application
	req = _req;
	conn = _conn;
	source = _source;
	ext = _ext;
}

void AsyncAPI::postEvent (Rts2Event *event)
{
	std::ostringstream os;
	switch (event->getType ())
	{
		case EVENT_COMMAND_OK:
			os << "{";
			req->sendConnectionValues (os, conn, NULL, rts2_nan("f"), ext);
			os << ",\"ret\":0 }";
			req->sendAsyncJSON (os, source);
			break;
		case EVENT_COMMAND_FAILED:
			os << "{";
			req->sendConnectionValues (os, conn, NULL, rts2_nan("f"), ext);
			os << ", \"ret\":-1 }";
			req->sendAsyncJSON (os, source);
			break;
	}
	Rts2Object::postEvent (event);
}

class AsyncAPIExpose:public AsyncAPI
{
	public:
		AsyncAPIExpose (API *_req, rts2core::Connection *conn, XmlRpcServerConnection *_source, bool _ext);

		virtual void postEvent (Rts2Event *event);
	private:
		enum {waitForExpReturn, waitForImage} callState;
};

AsyncAPIExpose::AsyncAPIExpose (API *_req, rts2core::Connection *_conn, XmlRpcServerConnection *_source, bool _ext):AsyncAPI (_req, _conn, _source, _ext)
{
	callState = waitForExpReturn;
}

void AsyncAPIExpose::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_COMMAND_OK:
			if (callState == waitForExpReturn)
				callState = waitForImage;
			break;
		case EVENT_COMMAND_FAILED:
			if (callState == waitForExpReturn)
			{
				std::ostringstream os;
				os << "{\"failed\"}";
				req->sendAsyncJSON (os, source);
			}
			break;
	}
	Rts2Object::postEvent (event);
}

API::API (const char* prefix, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, NULL, s)
{
}

void API::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	try
	{
		executeJSON (path, params, response_type, response, response_length);
	}
	catch (rts2core::Error &er)
	{
		throw JSONException (er.what ());
	}
}

void API::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;
	rts2core::Connection *conn;

	// widgets - divs with informations
	if (vals.size () >= 2 && vals[0] == "w")
	{
		getWidgets (vals, params, response_type, response, response_length);
		return;
	}

	os.precision (8);

	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	// calls returning binary data
	if (vals.size () == 1 && vals[0] == "lastimage")
	{
		const char *camera = params->getString ("ccd","");
		conn = master->getOpenConnection (camera);
		if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
			throw JSONException ("cannot find camera with given name");
		// XmlRpcd::createOtherType qurantee that the other connection is XmlDevCameraClient

		rts2image::Image *image = ((XmlDevCameraClient *) (conn->getOtherDevClient ()))->getActualImage ();
		if (image == NULL || image->getChannelSize () <= 0)
			throw JSONException ("camera did not take a single image");

		response_type = "binary/data";
		response_length = image->getPixelByteSize () * image->getChannelNPixels (0);
		response = new char[response_length];
		memcpy (response, image->getChannelData (0), response_length);
		return;
	}
	// calls returning arrays
	if (vals.size () == 1 && vals[0] == "devices")
	{
		os << "[";
		for (connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
		{
			if (iter != master->getConnections ()->begin ())
				os << ",";
			os << '"' << (*iter)->getName () << '"';
		}
		os << "]";
	}
	else if (vals.size () == 1 && vals[0] == "devbytype")
	{
		os << "[";
		int t = params->getInteger ("t",0);
		connections_t::iterator iter = master->getConnections ()->begin ();
		bool first = true;
		while (true)
		{
			master->getOpenConnectionType (t, iter);
			if (iter == master->getConnections ()->end ())
				break;
			if (first)
				first = false;
			else
				os << ",";
			os << '"' << (*iter)->getName () << '"';
			iter++;
		}
		os << ']';
	}
	// returns array with selection value strings
	else if (vals.size () == 1 && vals[0] == "selval")
	{
		const char *device = params->getString ("d","");
		const char *variable = params->getString ("n", "");
		if (variable[0] == '\0')
			throw JSONException ("variable name not set - missing or empty n parameter");
		if (isCentraldName (device))
			conn = master->getSingleCentralConn ();
		else
			conn = master->getOpenConnection (device);
		if (conn == NULL)
			throw JSONException ("cannot find device with given name");
		rts2core::Value * rts2v = master->getValue (device, variable);
		if (rts2v == NULL)
			throw JSONException ("cannot find variable");
		if (rts2v->getValueBaseType () != RTS2_VALUE_SELECTION)
			throw JSONException ("variable is not selection");
		sendSelection (os, (rts2core::ValueSelection *) rts2v);
	}
	// return sun altitude
	else if (vals.size () == 1 && vals[0] == "sunalt")
	{
		time_t from = params->getInteger ("from", master->getNow () - 86400);
		time_t to = params->getInteger ("to", from + 86400);
		const int steps = params->getInteger ("step", 1000);

		const double jd_from = ln_get_julian_from_timet (&from);
		const double jd_to = ln_get_julian_from_timet (&to);

		struct ln_equ_posn equ;
		struct ln_hrz_posn hrz;

		os << "[" << std::fixed;
		for (double jd = jd_from; jd < jd_to; jd += fabs (jd_to - jd_from) / steps)
		{
			if (jd != jd_from)
				os << ",";
			ln_get_solar_equ_coords (jd, &equ);
			ln_get_hrz_from_equ (&equ, Rts2Config::instance ()->getObserver (), jd, &hrz);
			os << "[" << hrz.alt << "," << hrz.az << "]";
		}
		os << "]";
	}
#ifdef HAVE_PGSQL
	else if (vals.size () == 1 && vals[0] == "script")
	{
		int id = params->getInteger ("id", -1);
		if (id <= 0)
			throw JSONException ("invalid id parameter");
		const char *cname = params->getString ("cn", "");
		if (cname[0] == '\0')
			throw JSONException ("empty camera name");
		
		rts2db::Target *target = createTarget (id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
		rts2script::Script script = rts2script::Script ();
		script.setTarget (cname, target);
		script.prettyPrint (os, rts2script::PRINT_JSON);
	}
	// return altitude of target..
	else if (vals.size () == 1 && vals[0] == "taltitudes")
	{
		int id = params->getInteger ("id", -1);
		if (id <= 0)
			throw JSONException ("invalid id parameter");
		time_t from = params->getInteger ("from", master->getNow () - 86400);
		time_t to = params->getInteger ("to", from + 86400);
		const int steps = params->getInteger ("steps", 1000);

		rts2db::Target *target = createTarget (id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
		const double jd_from = ln_get_julian_from_timet (&from);
		const double jd_to = ln_get_julian_from_timet (&to);

		struct ln_hrz_posn hrz;

		os << "[" << std::fixed;
		for (double jd = jd_from; jd < jd_to; jd += fabs (jd_to - jd_from) / steps)
		{
			if (jd != jd_from)
				os << ",";
			target->getAltAz (&hrz, jd);
			os << hrz.alt;
		}
		os << "]";
	}
#endif // HAVE_PGSQL
	else if (vals.size () == 1)
	{
		os << "{";
		if (vals[0] == "executor")
		{
			connections_t::iterator iter = master->getConnections ()->begin ();
			master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
			if (iter == master->getConnections ()->end ())
			 	throw JSONException ("executor is not connected");
			sendConnectionValues (os, *iter, params);
		}
		// device information
		else if (vals[0] == "deviceinfo")
		{
			const char *device = params->getString ("d","");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device with given name");
			os << "\"type\":" << conn->getOtherType ();
		}
		// set or increment variable
		else if (vals[0] == "set" || vals[0] == "inc" || vals[0] == "dec")
		{
			const char *device = params->getString ("d","");
			const char *variable = params->getString ("n", "");
			const char *value = params->getString ("v", "");
			int async = params->getInteger ("async", 0);
			int ext = params->getInteger ("e", 0);
			if (variable[0] == '\0')
				throw JSONException ("variable name not set - missing or empty n parameter");
			if (value[0] == '\0')
				throw JSONException ("value not set - missing or empty v parameter");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device with given name");
			rts2core::Value * rts2v = master->getValue (device, variable);
			if (rts2v == NULL)
				throw JSONException ("cannot find variable");
			char op;
			if (vals[0] == "inc")
				op = '+';
			else if (vals[0] == "dec")
				op = '-';
			else
				op = '=';
			if (async)
			{
				conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), std::string (variable), op, std::string (value), true));
				sendConnectionValues (os, conn, params, ext);
			}
			else
			{
				AsyncAPI *aa = new AsyncAPI (this, conn, connection, ext);
				((XmlRpcd *) getMasterApp ())->registerAPI (aa);

				conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), std::string (variable), op, std::string (value), true), 0, aa);
				throw XmlRpc::XmlRpcAsynchronous ();
			}

		}
		// return night start and end
		else if (vals[0] == "night")
		{
			double ns = params->getDouble ("day", time (NULL));
			time_t ns_t = ns;
			Rts2Night n (ln_get_julian_from_timet (&ns_t), Rts2Config::instance ()->getObserver ());
			os << "\"from\":" << *(n.getFrom ()) << ",\"to\":" << *(n.getTo ());
		}
		// get variable
		else if (vals[0] == "get" || vals[0] == "status")
		{
			const char *device = params->getString ("d","");
			bool ext = params->getInteger ("e", 0);
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device");
			double from = params->getDouble ("from", 0);
			sendConnectionValues (os, conn, params, from, ext);
		}
		// execute command on server
		else if (vals[0] == "cmd")
		{
			const char *device = params->getString ("d", "");
			const char *cmd = params->getString ("c", "");
			int async = params->getInteger ("async", 0);
			bool ext = params->getInteger ("e", 0);
			if (cmd[0] == '\0')
				throw JSONException ("empty command");
			if (isCentraldName (device))
				conn = master->getSingleCentralConn ();
			else
				conn = master->getOpenConnection (device);
			if (conn == NULL)
				throw JSONException ("cannot find device");
			if (async)
			{
				conn->queCommand (new rts2core::Command (master, cmd));
				sendConnectionValues (os, conn, params, rts2_nan("f"), ext);
			}
			else
			{
				AsyncAPI *aa = new AsyncAPI (this, conn, connection, ext);
				((XmlRpcd *) getMasterApp ())->registerAPI (aa);

				conn->queCommand (new rts2core::Command (master, cmd), 0, aa);
				throw XmlRpc::XmlRpcAsynchronous ();
			}
		}
		// start exposure, return from server image
		else if (vals[0] == "expose")
		{
			const char *camera = params->getString ("ccd","");
			bool ext = params->getInteger ("e", 0);
			conn = master->getOpenConnection (camera);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");

			// this is safe, as all DEVICE_TYPE_CCDs are XmlDevCameraClient classes
			XmlDevCameraClient *camdev = (XmlDevCameraClient *) conn->getOtherDevClient ();

			// this will throw exception if expand string was not yet used
			camdev->setNextExpand (params->getString ("fe", camdev->getDefaultFilename ()), false);

			AsyncAPI *aa = new AsyncAPI (this, conn, connection, ext);
			((XmlRpcd *) getMasterApp ())->registerAPI (aa);

			conn->queCommand (new rts2core::CommandExposure (master, camdev, 0), 0, aa);
			throw XmlRpc::XmlRpcAsynchronous ();
		}
		else if (vals[0] == "hasimage")
		{
			const char *camera = params->getString ("ccd","");
			conn = master->getOpenConnection (camera);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");
			// XmlRpcd::createOtherType qurantee that the other connection is XmlDevCameraClient
			rts2image::Image *image = ((XmlDevCameraClient *) (conn->getOtherDevClient ()))->getActualImage ();
			os << "\"hasimage\":" << ((image == NULL) ? "false" : "true");
		}
		else if (vals[0] == "runscript")
		{
			const char *d = params->getString ("d","");
			conn = master->getOpenConnection (d);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");
			const char *script = params->getString ("s","");
			bool kills = params->getInteger ("kill", 0);

			XmlDevCameraClient *camdev = (XmlDevCameraClient *) conn->getOtherDevClient ();

			camdev->setNextExpand (params->getString ("fe", camdev->getDefaultFilename ()), true);

			camdev->executeScript (script, kills);
			os << "\"d\":\"" << d << "\",\"s\":\"" << script << "\"";
		}
#ifdef HAVE_PGSQL
		// returns target information specified by target name
		else if (vals[0] == "tbyname")
		{
			rts2db::TargetSet tar_set;
			const char *name = params->getString ("n", "");
			bool ic = params->getInteger ("ic",1);
			bool pm = params->getInteger ("pm",1);
			if (name[0] == '\0')
				throw JSONException ("empty n parameter");
			tar_set.loadByName (name, pm, ic);
			jsonTargets (tar_set, os, params);
		}
		// returns target specified by target ID
		else if (vals[0] == "tbyid")
		{
			rts2db::TargetSet tar_set;
			int id = params->getInteger ("id", -1);
			if (id <= 0)
				throw JSONException ("empty id parameter");
			tar_set.load (id);
			jsonTargets (tar_set, os, params);
		}
		// returns target with given label
		else if (vals[0] == "tbylabel")
		{
			rts2db::TargetSet tar_set;
			int label = params->getInteger ("l", -1);
			if (label == -1)
			  	throw JSONException ("empty l parameter");
			tar_set.loadByLabelId (label);
			jsonTargets (tar_set, os, params);
		}
		// returns targets within certain radius from given ra dec
		else if (vals[0] == "tbydistance")
		{
			struct ln_equ_posn pos;
			pos.ra = params->getDouble ("ra",rts2_nan("f"));
			pos.dec = params->getDouble ("dec",rts2_nan("f"));
			double radius = params->getDouble ("radius",rts2_nan("f"));
			if (isnan (pos.ra) || isnan (pos.dec) || isnan (radius))
				throw JSONException ("invalid ra, dec or radius parameter");
			rts2db::TargetSet ts (&pos, radius, Rts2Config::instance ()->getObserver ());
			ts.load ();
			jsonTargets (ts, os, params, &pos);
		}
		// try to parse and understand string (similar to new target), return either target or all target information
		else if (vals[0] == "tbystring")
		{
			const char *tar_name = params->getString ("ts", "");
			if (tar_name[0] == '\0')
				throw JSONException ("empty ts parameter");
			rts2db::Target *target = createTargetByString (tar_name);
			if (target == NULL)
				throw JSONException ("cannot parse target");
			struct ln_equ_posn pos;
			target->getPosition (&pos);
			os << "\"name\":\"" << tar_name << "\",\"ra\":" << pos.ra << ",\"dec\":" << pos.dec << ",\"desc\":\"" << target->getTargetInfo () << "\"";
			double nearest = params->getDouble ("nearest", -1);
			if (nearest >= 0)
			{
				os << ",\"nearest\":{";
				rts2db::TargetSet ts (&pos, nearest, Rts2Config::instance ()->getObserver ());
				ts.load ();
				jsonTargets (ts, os, params, &pos);
				os << "}";
			}
		}
		else if (vals[0] == "ibyoid")
		{
			int obsid = params->getInteger ("oid", -1);
			if (obsid == -1)
				throw JSONException ("empty oid parameter");
			rts2db::Observation obs (obsid);
			if (obs.load ())
				throw JSONException ("Cannot load observation set");
			obs.loadImages ();	
			jsonImages (obs.getImageSet (), os, params);
		}
		else if (vals[0] == "labels")
		{
			const char *label = params->getString ("l", "");
			if (label[0] == '\0')
				throw JSONException ("empty l parameter");
			int t = params->getInteger ("t", -1);
			if (t < 0)
				throw JSONException ("invalid type parametr");
			rts2db::Labels lb;
			os << lb.getLabel (label, t);
		}
		else if (vals[0] == "consts")
		{
			int tid = params->getInteger ("id", -1);
			if (tid <= 0)
				throw JSONException ("empty target ID");
			rts2db::Target *target = createTarget (tid, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			target->getConstraints ()->printJSON (os);
		}
		// violated constrainsts..
		else if (vals[0] == "violated")
		{
			const char *cn = params->getString ("consts", "");
			if (cn[0] == '\0')
				throw JSONException ("unknow constraint name");
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknown target ID");
			double from = params->getDouble ("from", master->getNow ());
			double to = params->getDouble ("to", from + 86400);
			// 60 sec = 1 minute step (by default)
			double step = params->getDouble ("step", 60);

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), master->getNotifyConnection ());
			rts2db::Constraints *cons = tar->getConstraints ();

			os << '"' << cn << "\":[";

			if (cons->find (std::string (cn)) != cons->end ())
			{
				rts2db::ConstraintPtr cptr = (*cons)[std::string (cn)];
				bool first_it = true;

				rts2db::interval_arr_t intervals;
				cptr->getViolatedIntervals (tar, from, to, step, intervals);
				for (rts2db::interval_arr_t::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
				{
					if (first_it)
						first_it = false;
					else
						os << ",";
					os << "[" << iter->first << "," << iter->second << "]";
				}
			}
			os << "]";
		}
		// find unviolated time interval..
		else if (vals[0] == "satisfied")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			time_t from = params->getDouble ("from", master->getNow ());
			time_t to = params->getDouble ("to", from + 86400);
			double length = params->getDouble ("length", rts2_nan ("f"));
			int step = params->getInteger ("step", 60);

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			if (isnan (length))
				length = 1800;
			rts2db::interval_arr_t si;
			from -= from % step;
			to += step - (to % step);
			tar->getSatisfiedIntervals (from, to, length, step, si);
			os << "\"id\":" << tar_id << ",\"satisfied\":[";
			for (rts2db::interval_arr_t::iterator sat = si.begin (); sat != si.end (); sat++)
			{
				if (sat != si.begin ())
					os << ",";
				os << "[" << sat->first << "," << sat->second << "]";
			}
			os << "]";
		}
		// return intervals of altitude constraints (or violated intervals)
		else if (vals[0] == "cnst_alt" || vals[0] == "cnst_alt_v")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());

			std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > ac;

			if (vals[0] == "cnst_alt")
				tar->getAltitudeConstraints (ac);
			else
				tar->getAltitudeViolatedConstraints (ac);

			os << "\"id\":" << tar_id << ",\"altitudes\":{";

			for (std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> >::iterator iter = ac.begin (); iter != ac.end (); iter++)
			{
				if (iter != ac.begin ())
					os << ",\"";
				else
					os << "\"";
				os << iter->first << "\":[";
				for (std::vector <rts2db::ConstraintDoubleInterval>::iterator di = iter->second.begin (); di != iter->second.end (); di++)
				{
					if (di != iter->second.begin ())
						os << ",[";
					else
						os << "[";
					os << JsonDouble (di->getLower ()) << "," << JsonDouble (di->getUpper ()) << "]";
				}
				os << "]";
			}
			os << "}";
		}
		// return intervals of time constraints (or violated time intervals)
		else if (vals[0] == "cnst_time" || vals[0] == "cnst_time_v")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			time_t from = params->getInteger ("from", master->getNow ());
			time_t to = params->getInteger ("to", from + 86400);
			double steps = params->getDouble ("steps", 1000);

			steps = double ((to - from)) / steps;

			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());

			std::map <std::string, rts2db::ConstraintPtr> cons;

			tar->getTimeConstraints (cons);

			os << "\"id\":" << tar_id << ",\"constraints\":{";

			for (std::map <std::string, rts2db::ConstraintPtr>::iterator iter = cons.begin (); iter != cons.end (); iter++)
			{
				if (iter != cons.begin ())
					os << ",\"";
				else
					os << "\"";
				os << iter->first << "\":[";

				rts2db::interval_arr_t intervals;
				if (vals[0] == "cnst_time")
					iter->second->getSatisfiedIntervals (tar, from, to, steps, intervals);
				else
					iter->second->getViolatedIntervals (tar, from, to, steps, intervals);

				for (rts2db::interval_arr_t::iterator it = intervals.begin (); it != intervals.end (); it++)
				{
					if (it != intervals.begin ())
						os << ",[";
					else
						os << "[";
					os << it->first << "," << it->second << "]";
				}
				os << "]";
			}
			os << "}";
		}
		else if (vals[0] == "create_target")
		{
			const char *tn = params->getString ("tn", "");
			double ra = params->getDouble ("ra", rts2_nan("f"));
			double dec = params->getDouble ("dec", rts2_nan("f"));
			const char *desc = params->getString ("desc", "");
			const char *type = params->getString ("type", "O");

			if (strlen (tn) == 0)
				throw JSONException ("empty target name");
			if (strlen (type) != 1)
				throw JSONException ("invalid target type");

			rts2db::ConstTarget nt;
			nt.setTargetName (tn);
			nt.setPosition (ra, dec);
			nt.setTargetInfo (std::string (desc));
			nt.setTargetType (type[0]);
			nt.save (false);

			os << "\"id\":" << nt.getTargetID ();
		}
		else if (vals[0] == "update_target")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *t = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			switch (t->getTargetType ())
			{
				case TYPE_OPORTUNITY:
				case TYPE_GRB:
				case TYPE_FLAT:
				case TYPE_CALIBRATION:
				case TYPE_FOCUSING:
				case TYPE_GPS:
				case TYPE_TERESTIAL:
				case TYPE_AUGER:
				case TYPE_LANDOLT:
				{
					rts2db::ConstTarget *tar = (rts2db::ConstTarget *) t;
					const char *tn = params->getString ("tn", "");
					double ra = params->getDouble ("ra", rts2_nan("f"));
					double dec = params->getDouble ("dec", rts2_nan("f"));
					const char *desc = params->getString ("desc", NULL);

					if (strlen (tn) > 0)
						tar->setTargetName (tn);
					tar->setPosition (ra, dec);
					if (desc != NULL)
						tar->setTargetInfo (std::string (desc));
					tar->save (true);
		
					os << "\"id\":" << tar->getTargetID ();
					delete tar;
					break;
				}	
				default:
				{
					std::ostringstream _err;
					_err << "can update only subclass of constant targets, " << t->getTargetType () << " is unknow";
					throw JSONException (_err.str ());
				}	
			}		

		}
		else if (vals[0] == "change_script")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			const char *cam = params->getString ("c", NULL);
			if (strlen (cam) == 0)
				throw JSONException ("unknow camera");
			const char *s = params->getString ("s", "");
			if (strlen (s) == 0)
				throw JSONException ("empty script");
			tar->setScript (cam, s);
			os << "\"id\":" << tar_id << ",\"camera\":\"" << cam << "\",\"script\":\"" << s << "\"";
			delete tar;
		}
		else if (vals[0] == "change_constraints")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			const char *cn = params->getString ("cn", NULL);
			const char *ci = params->getString ("ci", NULL);
			if (cn == NULL)
				throw JSONException ("constraint not specified");
			rts2db::Constraints constraints;
			constraints.parse (cn, ci);
			tar->appendConstraints (constraints);

			os << "\"id\":" << tar_id << ",";
			constraints.printJSON (os);

			delete tar;
		}
		else if (vals[0] == "tlabs_delete")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			int ltype = params->getInteger ("ltype", -1);
			if (ltype < 0)
				throw JSONException ("unknow/missing label type");
			tar->deleteLabels (ltype);
			jsonLabels (tar, os);
		}
		else if (vals[0] == "tlabs_add" || vals[0] == "tlabs_set")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
			int ltype = params->getInteger ("ltype", -1);
			if (ltype < 0)
				throw JSONException ("unknow/missing label type");
			const char *ltext = params->getString ("ltext", NULL);
			if (ltext == NULL)
				throw JSONException ("missing label text");
			if (vals[0] == "tlabs_set")
				tar->deleteLabels (ltype);
			tar->addLabel (ltext, ltype, true);
			jsonLabels (tar, os);
		}
		else if (vals[0] == "obytid")
		{
			int tar_id = params->getInteger ("id", -1);
			if (tar_id < 0)
				throw JSONException ("unknow target ID");
			rts2db::ObservationSet obss = rts2db::ObservationSet ();

			obss.loadTarget (tar_id);

			jsonObservations (&obss, os);
			
		}
		else if (vals[0] == "plan")
		{
			rts2db::PlanSet ps (params->getDouble ("from", master->getNow ()), params->getDouble ("to", rts2_nan ("f")));
			ps.load ();

			os << "\"h\":["
				"{\"n\":\"Plan ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/plan/\",\"href\":0,\"c\":0},"
				"{\"n\":\"Target ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/targets/\",\"href\":1,\"c\":1},"
				"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/targets/\",\"href\":1,\"c\":2},"
				"{\"n\":\"Obs ID\",\"t\":\"a\",\"prefix\":\"" << master->getPagePrefix () << "/observations/\",\"href\":3,\"c\":3},"
				"{\"n\":\"Start\",\"t\":\"t\",\"c\":4},"
				"{\"n\":\"End\",\"t\":\"t\",\"c\":5},"
				"{\"n\":\"RA\",\"t\":\"r\",\"c\":6},"
				"{\"n\":\"DEC\",\"t\":\"d\",\"c\":7},"
				"{\"n\":\"Alt start\",\"t\":\"altD\",\"c\":8},"
				"{\"n\":\"Az start\",\"t\":\"azD\",\"c\":9}],"
				"\"d\":[" << std::fixed;

			for (rts2db::PlanSet::iterator iter = ps.begin (); iter != ps.end (); iter++)
			{
				if (iter != ps.begin ())
					os << ",";
				rts2db::Target *tar = iter->getTarget ();
				time_t t = iter->getPlanStart ();
				double JDstart = ln_get_julian_from_timet (&t);
				struct ln_equ_posn equ;
				tar->getPosition (&equ, JDstart);
				struct ln_hrz_posn hrz;
				tar->getAltAz (&hrz, JDstart);
				os << "[" << iter->getPlanId () << ","
					<< iter->getTargetId () << ",\""
					<< tar->getTargetName () << "\","
					<< iter->getObsId () << ",\""
					<< iter->getPlanStart () << "\",\""
					<< iter->getPlanEnd () << "\","
					<< equ.ra << "," << equ.dec << ","
					<< hrz.alt << "," << hrz.az << "]";
			}

			os << "]";
		}
		else if (vals[0] == "labellist")
		{
			rts2db::LabelList ll;
			ll.load ();

			os << "\"h\":["
				"{\"n\":\"Label ID\",\"t\":\"n\",\"c\":0},"
				"{\"n\":\"Label type\",\"t\":\"n\",\"c\":1},"
				"{\"n\":\"Label text\",\"t\":\"s\",\"c\":2}],"
				"\"d\":[";

			for (rts2db::LabelList::iterator iter = ll.begin (); iter != ll.end (); iter++)
			{
				if (iter != ll.begin ())
					os << ",";
				os << "[" << iter->labid << ","
					<< iter->tid << ",\""
					<< iter->text << "\"]";
			}
			os << "]";
		}
		else if (vals[0] == "messages")
		{
			double to = params->getDouble ("to", master->getNow ());
			double from = params->getDouble ("from", to - 86400);
			int typemask = params->getInteger ("type", MESSAGE_MASK_ALL);

			rts2db::MessageSet ms;
			ms.load (from, to, typemask);

			os << "\"h\":["
				"{\"n\":\"Time\",\"t\":\"t\",\"c\":0},"
				"{\"n\":\"Component\",\"t\":\"s\",\"c\":1},"
				"{\"n\":\"Type\",\"t\":\"n\",\"c\":2},"
				"{\"n\":\"Text\",\"t\":\"s\",\"c\":3}],"
				"\"d\":[";

			for (rts2db::MessageSet::iterator iter = ms.begin (); iter != ms.end (); iter++)
			{
				if (iter != ms.begin ())
					os << ",";
				os << "[" << iter->getMessageTime () << ",\"" << iter->getMessageOName () << "\"," << iter->getType () << ",\"" << iter->getMessageString () << "\"]";
			}
			os << "]";
		}
#endif
		else
		{
			throw JSONException ("invalid request " + path);
		}
		os << "}";
	}

	returnJSON (os.str ().c_str (), response_type, response, response_length);
}

void API::sendArrayValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
			for (std::vector <int>::iterator iter = ((rts2core::IntegerArray *) value)->valueBegin (); iter != ((rts2core::IntegerArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::IntegerArray *) value)->valueBegin ())
					os << ",";
				os << (*iter);
			}
			break;
		case RTS2_VALUE_STRING:
			for (std::vector <std::string>::iterator iter = ((rts2core::StringArray *) value)->valueBegin (); iter != ((rts2core::StringArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::StringArray *) value)->valueBegin ())
					os << ",";
				os << "\"" << (*iter) << "\"";
			}
			break;
		case RTS2_VALUE_DOUBLE:	
		case RTS2_VALUE_TIME:
			for (std::vector <double>::iterator iter = ((rts2core::DoubleArray *) value)->valueBegin (); iter != ((rts2core::DoubleArray *) value)->valueEnd (); iter++)
			{
				if (iter != ((rts2core::DoubleArray *) value)->valueBegin ())
					os << ",";
				os << JsonDouble (*iter);
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void API::sendStatValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
			{
				rts2core::ValueDoubleStat *vds = (rts2core::ValueDoubleStat *) value;
				os << vds->getNumMes () << ","
					<< JsonDouble (vds->getValueDouble ()) << ","
					<< JsonDouble (vds->getMode ()) << ","
					<< JsonDouble (vds->getStdev ()) << ","
					<< JsonDouble (vds->getMin ()) << ","
					<< JsonDouble (vds->getMax ());
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void API::sendRectangleValue (rts2core::Value *value, std::ostringstream &os)
{
	rts2core::ValueRectangle *r = (rts2core::ValueRectangle *) value;
	os << "[" << r->getXInt () << "," << r->getYInt () << "," << r->getWidthInt () << "," << r->getHeightInt () << "]";
}

void API::sendValue (rts2core::Value *value, std::ostringstream &os)
{
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_STRING:
			os << "\"" << value->getValue () << "\"";
			break;
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_TIME:
			os << JsonDouble (value->getValueDouble ());
			break;
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
		case RTS2_VALUE_SELECTION:
		case RTS2_VALUE_BOOL:
			os << value->getValue ();
			break;
		case RTS2_VALUE_RADEC:
			os << "{\"ra\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getRa ()) << ",\"dec\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getDec ()) << "}";
			break;
		case RTS2_VALUE_ALTAZ:
			os << "{\"alt\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAlt ()) << ",\"az\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAz ()) << "}";
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
}

void API::sendSelection (std::ostringstream &os, rts2core::ValueSelection *value)
{
	os << "[";
	for (std::vector <rts2core::SelVal>::iterator iter = value->selBegin (); iter != value->selEnd (); iter++)
	{
		if (iter != value->selBegin ())
			os << ",";
		os << '"' << iter->name << '"';
	}
	os << "]";

}

void API::sendConnectionValues (std::ostringstream & os, rts2core::Connection * conn, HttpParams *params, double from, bool extended)
{
	os << "\"d\":{" << std::fixed;
	double mfrom = rts2_nan ("f");
	bool first = true;
	rts2core::ValueVector::iterator iter;

	for (iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		if (conn->getOtherDevClient ())
		{
			double ch = ((XmlDevInterface *) (conn->getOtherDevClient ()))->getValueChangedTime (*iter);
			if (isnan (mfrom) || ch > mfrom)
				mfrom = ch;
			if (!isnan (from) && !isnan (ch) && ch < from)
				continue;
		}

		if (first)
			first = false;
		else
			os << ",";

		os << "\"" << (*iter)->getName () << "\":";
		if (extended)
			os << "[" << (*iter)->getFlags () << ",";
	  	if ((*iter)->getValueExtType() & RTS2_VALUE_ARRAY)
		  	sendArrayValue (*iter, os);
		else switch ((*iter)->getValueExtType())
		{
			case RTS2_VALUE_STAT:
				sendStatValue (*iter, os);
				break;
			case RTS2_VALUE_RECTANGLE:
				sendRectangleValue (*iter, os);
				break;
			default:
		  		sendValue (*iter, os);
				break;
		}
		if (extended)
			os << "," << (*iter)->isError () << "," << (*iter)->isWarning () << ",\"" << (*iter)->getDescription () << "\"]";
	}
	os << "},\"minmax\":{";

	bool firstMMax = true;

	for (iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		if ((*iter)->getValueExtType () == RTS2_VALUE_MMAX && (*iter)->getValueBaseType () == RTS2_VALUE_DOUBLE)
		{
			rts2core::ValueDoubleMinMax *v = (rts2core::ValueDoubleMinMax *) (*iter);
			if (firstMMax)
				firstMMax = false;
			else
				os << ",";
			os << "\"" << v->getName () << "\":[" << JsonDouble (v->getMin ()) << "," << JsonDouble (v->getMax ()) << "]";
		}
	}

	os << "},\"idle\":" << conn->isIdle () << ",\"state\":" << conn->getState () << ",\"f\":" << JsonDouble (mfrom);
}

void API::getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream os;

	includeJavaScript (os, "widgets.js");

	if (vals[1] == "executor")
	{
		os << "<div id='executor'>\n<table>"
			"<tr><td>Current</td><td><div id='ex:current'></div></td></tr>"
			"<tr><td>Next</td><td><div id='ex:next'></div></td></tr>"
			"</table>\n"
			"<button type='button' onClick=\"refresh('executor','ex:','executor');\">Refresh</button>\n"
			"</div>\n";
	}
	else
	{
		throw XmlRpcException ("Widget " + vals[1] + " is not defined.");
	}

	response_type = "text/html";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}

#ifdef HAVE_PGSQL
void API::jsonTargets (rts2db::TargetSet &tar_set, std::ostream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom)
{
	bool extended = params->getInteger ("e", false);
	time_t from = params->getInteger ("from", getMasterApp()->getNow ());
	int c = 4;
	os << "\"h\":["
		"{\"n\":\"Target ID\",\"t\":\"n\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":0},"
		"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":1},"
		"{\"n\":\"RA\",\"t\":\"r\",\"c\":2},"
		"{\"n\":\"DEC\",\"t\":\"d\",\"c\":3},"
		"{\"n\":\"Description\",\"t\":\"s\",\"c\":4}";
		
	
	struct ln_equ_posn oradec;

	if (dfrom == NULL)
	{
			oradec.ra = params->getDouble ("ra",rts2_nan("f"));
			oradec.dec = params->getDouble ("dec",rts2_nan("f"));
			if (!isnan (oradec.ra) && !isnan (oradec.dec))
				dfrom = &oradec;
	}
	if (dfrom)
	{
		os << ",{\"n\":\"Distance\",\"t\":\"d\",\"c\":4}";
		c = 5;
	}
	if (extended)
		os << ",{\"n\":\"Duration\",\"t\":\"dur\",\"c\":" << (c) << "},"
		"{\"n\":\"Scripts\",\"t\":\"scripts\",\"c\":" << (c + 1) << "},"
		"{\"n\":\"Satisfied\",\"t\":\"s\",\"c\":" << (c + 2) << "},"
		"{\"n\":\"Violated\",\"t\":\"s\",\"c\":" << (c + 3) << "},"
		"{\"n\":\"Transit\",\"t\":\"t\",\"c\":" << (c + 4) << "},"
		"{\"n\":\"Observable\",\"t\":\"t\",\"c\":" << (c + 5) << "}";
	os << "],\"d\":[" << std::fixed;

	double JD = ln_get_julian_from_timet (&from);
	for (rts2db::TargetSet::iterator iter = tar_set.begin (); iter != tar_set.end (); iter++)
	{
		if (iter != tar_set.begin ())
			os << ",";
		struct ln_equ_posn equ;
		rts2db::Target *tar = iter->second;
		tar->getPosition (&equ, JD);
		const char *n = tar->getTargetName ();
		os << "[" << tar->getTargetID () << ",";
		if (n == NULL)
			os << "null,";
		else
			os << "\"" << n << "\",";

		os << JsonDouble (equ.ra) << ',' << JsonDouble (equ.dec) << ",\"" << tar->getTargetInfo () << "\"";

		if (dfrom)
			os << ',' << JsonDouble (tar->getDistance (dfrom, JD));

		if (extended)
		{
			double md = -1;
			std::ostringstream cs;
			for (Rts2CamList::iterator cam = ((XmlRpcd *) getMasterApp ())->cameras.begin (); cam != ((XmlRpcd *) getMasterApp ())->cameras.end (); cam++)
			{
				try
				{
					std::string script_buf;
					rts2script::Script script;
					tar->getScript (cam->c_str(), script_buf);
					if (cam != ((XmlRpcd *) getMasterApp ())->cameras.begin ())
						cs << ",";
					script.setTarget (cam->c_str (), tar);
					double d = script.getExpectedDuration ();
					int e = script.getExpectedImages ();
					cs << "{\"" << *cam << "\":[\"" << script_buf << "\"," << d << "," << e << "]}";
					if (d > md)
						md = d;  
				}
				catch (rts2core::Error &er)
				{
					logStream (MESSAGE_ERROR) << "cannot parsing script for camera " << *cam << ": " << er << sendLog;
				}
			}
			os << "," << md << ",[" << cs.str () << "],";

			tar->getSatisfiedConstraints (JD).printJSON (os);
			
			os << ",";
		
			tar->getViolatedConstraints (JD).printJSON (os);

			if (isnan (equ.ra) || isnan (equ.dec))
			{
				os << ",null,true";
			}
			else
			{
				struct ln_hrz_posn hrz;
				tar->getAltAz (&hrz, JD);

				struct ln_rst_time rst;
				tar->getRST (&rst, JD, 0);

				os << "," << JsonDouble (rst.transit) 
					<< "," << (tar->isAboveHorizon (&hrz) ? "true" : "false");
			}
		}
		os << "]";
	}
	os << "]";
}

void API::jsonObservations (rts2db::ObservationSet *obss, std::ostream &os)
{
	os << "\"h\":["
		"{\"n\":\"ID\",\"t\":\"n\",\"c\":0,\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/observations/\",\"href\":0},"
		"{\"n\":\"Slew\",\"t\":\"t\",\"c\":1},"
		"{\"n\":\"Start\",\"t\":\"tT\",\"c\":2},"
		"{\"n\":\"End\",\"t\":\"tT\",\"c\":3},"
		"{\"n\":\"Number of images\",\"t\":\"n\",\"c\":4},"
		"{\"n\":\"Number of good images\",\"t\":\"n\",\"c\":5}"
		"],\"d\":[";

	os << std::fixed;

	for (rts2db::ObservationSet::iterator iter = obss->begin (); iter != obss->end (); iter++)
	{
		if (iter != obss->begin ())
			os << ",";
		os << "[" << iter->getObsId () << ","
			<< JsonDouble(iter->getObsSlew ()) << ","
			<< JsonDouble(iter->getObsStart ()) << ","
			<< JsonDouble(iter->getObsEnd ()) << ","
			<< iter->getNumberOfImages () << ","
			<< iter->getNumberOfGoodImages ()
			<< "]";
	}

	os << "]";
}

void API::jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params)
{
	os << "\"h\":["
		"{\"n\":\"Image\",\"t\":\"ccdi\",\"c\":0},"
		"{\"n\":\"Start\",\"t\":\"tT\",\"c\":1},"
		"{\"n\":\"Exptime\",\"t\":\"dur\",\"c\":2},"
		"{\"n\":\"Filter\",\"t\":\"s\",\"c\":3},"
		"{\"n\":\"Path\",\"t\":\"ip\",\"c\":4}"
	"],\"d\":[" << std::fixed;

	for (rts2db::ImageSet::iterator iter = img_set->begin (); iter != img_set->end (); iter++)
	{
		if (iter != img_set->begin ())
			os << ",";
		os << "[\"" << (*iter)->getFileName () << "\","
			<< JsonDouble ((*iter)->getExposureSec () + (*iter)->getExposureUsec () / USEC_SEC) << ","
			<< JsonDouble ((*iter)->getExposureLength ()) << ",\""
			<< (*iter)->getFilter () << "\",\""
			<< (*iter)->getAbsoluteFileName () << "\"]";
	}

	os << "]";
}

void API::jsonLabels (rts2db::Target *tar, std::ostream &os)
{
	std::vector <std::pair <int, std::string> > tlabels = tar->getLabels ();
	os << "\"id\":" << tar->getTargetID () << ",\"labels\":[";
	for (std::vector <std::pair <int, std::string> >::iterator iter = tlabels.begin (); iter != tlabels.end (); iter++)
	{
		if (iter == tlabels.begin ())
			os << "[";
		else
			os << ",[";
		os << iter->first << ",\"" << iter->second << "\"]";
	}
	os << "]";
}
#endif // HAVE_PGSQL
