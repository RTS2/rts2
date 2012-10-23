/* 
 * API access for RTS2.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011,2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
 * @section Propagation of call through RTS2.
 *
 * JSON calls need to be propagated through RTS2 system. Response to call must
 * be provided only after it becomes available from RTS2 subsystems. The
 * following diagram provides an overview how the API calls are handled.
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
 * document with error decription. For example call of \ref JSON_device_set
 * with device argument pointing to device not present in the system will
 * return this error.
 *
 * Calls to non-existent points returns HTTP error code.
 *
 * @section JSON_API_calls JSON API calls
 *
 * The calls are divided into two categories - device and SQL (database) calls.
 * Detailed description is provided on two pages:
 *
 *  - @ref JSON_device
 *  - @ref JSON_sql
 */

#include "xmlrpcd.h"
#include "rts2json/jsonvalue.h"

#include "rts2db/constraints.h"
#include "rts2db/planset.h"
#include "rts2script/script.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/labellist.h"
#include "rts2db/simbadtarget.h"
#include "rts2db/messagedb.h"
#include "rts2db/target_auger.h"
#endif

using namespace rts2xmlrpc;

void getCameraParameters (XmlRpc::HttpParams *params, const char *&camera, long &smin, long &smax, rts2image::scaling_type &scaling, int &newType)
{
	camera = params->getString ("ccd","");
	smin = params->getLong ("smin", LONG_MIN);
	smax = params->getLong ("smax", LONG_MAX);

	const char *scalings[] = { "lin", "log", "sqrt", "pow" };
	const char *sc = params->getString ("scaling", "");
	scaling = rts2image::SCALING_LINEAR;
	if (sc[0] != '\0')
	{
		for (int i = 0; i < 3; i++)
		{
			if (!strcasecmp (sc, scalings[i]))
			{
				scaling = (rts2image::scaling_type) i;
				break;
			}
		}
	}

	newType = params->getInteger ("2data", 0);
}

/** Camera API classes */

API::API (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::GetRequestAuthorized (prefix, _http_server, NULL, s)
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

void API::authorizePage (int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	throw JSONException ("cannot authorise user", HTTP_UNAUTHORIZED);
}

void sendSelection (std::ostringstream &os, rts2core::ValueSelection *value)
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

void jsonObservation (rts2db::Observation *obs, std::ostream &os);

void API::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;
	rts2core::Connection *conn = NULL;

	// widgets - divs with informations
	if (vals.size () >= 2 && vals[0] == "w")
	{
		getWidgets (vals, params, response_type, response, response_length);
		return;
	}

	os.precision (8);

	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	// calls returning binary data
	if (vals.size () == 1 && (vals[0] == "currentimage" || vals[0] == "lastimage"))
	{
		const char *camera;
		long smin, smax;
		rts2image::scaling_type scaling;
		int newType;
		getCameraParameters (params, camera, smin, smax, scaling, newType);

		conn = master->getOpenConnection (camera);
		if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
			throw JSONException ("cannot find camera with given name");
		int chan = params->getInteger ("chan", 0);

		if (vals[0] == "currentimage")
		{
			// XmlRpcd::createOtherType qurantee that the other connection is XmlDevCameraClient
			// first try if there is data connection opened, so image data should be streamed in
			if (DataAbstractRead * lastData = conn->lastDataChannel (chan))
			{
				AsyncCurrentAPI *aa = new AsyncCurrentAPI (this, conn, connection, lastData, chan, smin, smax, scaling, newType);
				((XmlRpcd *) getMasterApp ())->registerAPI (aa);

				throw XmlRpc::XmlRpcAsynchronous ();
			}
		}

		// XmlRpcd::createOtherType qurantee that the other connection is XmlDevCameraClient
		rts2image::Image *image = ((XmlDevCameraClient *) (conn->getOtherDevClient ()))->getPreviousImage ();
		if (image == NULL)
			throw JSONException ("camera did not take a single image");
		if (image->getChannelSize () <= chan)
			throw JSONException ("cannot find specified channel");

		imghdr im_h;
		image->getImgHeader (&im_h, chan);
		if (abs (ntohs (im_h.data_type)) < abs (newType))
			throw JSONException ("data type specified for scaling is bigger than actual image data type");
		if (newType != 0 && ((ntohs (im_h.data_type) < 0 && newType > 0) || (ntohs (im_h.data_type) > 0 && newType < 0)))
			throw JSONException ("converting from integer into float type, or from float to integer");

		response_type = "binary/data";
		response_length = image->getPixelByteSize () * image->getChannelNPixels (chan);
		if (newType != 0)
			response_length /= (ntohs (im_h.data_type) / newType);

		response_length += sizeof (imghdr);

		response = new char[response_length];

		if (newType != 0)
		{
			memcpy (response + sizeof (imghdr), image->getChannelDataScaled (chan, smin, smax, scaling, newType), response_length - sizeof (imghdr));
			im_h.data_type = htons (newType);
		}
		else
		{
			memcpy (response + sizeof (imghdr), image->getChannelData (chan), response_length - sizeof (imghdr));
		}
		memcpy (response, &im_h, sizeof (imghdr));
		return;
	}
	// calls returning arrays
	else if (vals.size () == 1 && vals[0] == "devices")
	{
		int ext = params->getInteger ("e", 0);
		os << "[";
		if (ext)
			os << "[\"" << ((XmlRpcd *) getMasterApp ())->getDeviceName () << "\"," << DEVICE_TYPE_XMLRPC << ']';
		else
			os << '"' << ((XmlRpcd *) getMasterApp ())->getDeviceName () << '"';
		for (connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
		{
			if ((*iter)->getName ()[0] == '\0')
				continue;
			if (ext)
				os << ",[\"" << (*iter)->getName () << "\"," << (*iter)->getOtherType () << ']';
			else
				os << ",\"" << (*iter)->getName () << '"';
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
		time_t from = params->getInteger ("from", getNow () - 86400);
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
			ln_get_hrz_from_equ (&equ, Configuration::instance ()->getObserver (), jd, &hrz);
			os << "[" << hrz.alt << "," << hrz.az << "]";
		}
		os << "]";
	}
#ifdef RTS2_HAVE_PGSQL
	else if (vals.size () == 1 && vals[0] == "script")
	{
		rts2db::Target *target = getTarget (params);
		const char *cname = params->getString ("cn", "");
		if (cname[0] == '\0')
			throw JSONException ("empty camera name");
		
		rts2script::Script script = rts2script::Script ();
		script.setTarget (cname, target);
		script.prettyPrint (os, rts2script::PRINT_JSON);
	}
	// return altitude of target..
	else if (vals.size () == 1 && vals[0] == "taltitudes")
	{
		time_t from = params->getInteger ("from", getNow () - 86400);
		time_t to = params->getInteger ("to", from + 86400);
		const int steps = params->getInteger ("steps", 1000);

		rts2db::Target *target = getTarget (params);

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
#endif // RTS2_HAVE_PGSQL
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
			char op;
			if (vals[0] == "inc")
				op = '+';
			else if (vals[0] == "dec")
				op = '-';
			else
				op = '=';
			// own connection
			if (!(strcmp (device, ((XmlRpcd *) getMasterApp ())->getDeviceName ())))
			{
				((XmlRpcd *) getMasterApp ())->doOpValue (variable, op, value);
				sendOwnValues (os, params, NAN, ext);
			}
			else
			{
				if (isCentraldName (device))
					conn = master->getSingleCentralConn ();
				else
					conn = master->getOpenConnection (device);
				if (conn == NULL)
					throw JSONException ("cannot find device with given name");
				rts2core::Value * rts2v = master->getValue (device, variable);
				if (rts2v == NULL)
					throw JSONException ("cannot find variable");
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

		}
		else if (vals[0] == "statadd")
		{
			const char *vn = params->getString ("n","");
			if (strlen (vn) == 0)
				throw rts2core::Error ("empty n parameter");
			int num = params->getInteger ("num",-1);
			if (num <= 0)
				throw rts2core::Error ("missing or negative num parameter");
			double v = params->getDouble ("v", NAN);
			if (isnan (v))
				throw rts2core::Error ("missing or invalid v parameter");
			rts2core::Value *val = ((XmlRpcd *) getMasterApp())->getOwnValue (vn);
			if (val == NULL)
				throw rts2core::Error (std::string ("cannot find variable with name ") + vn);
			if (!(val->getValueExtType () == RTS2_VALUE_STAT && val->getValueBaseType () == RTS2_VALUE_DOUBLE))
				throw rts2core::Error ("value is not statistical double type");
			((rts2core::ValueDoubleStat *) val)->addValue (v, num);
			((XmlRpcd *)getMasterApp ())->sendValueAll (val);
			sendOwnValues (os, params, NAN, false);
		}
		else if (vals[0] == "statclear")
		{
			const char *vn = params->getString ("n","");
			if (strlen (vn) == 0)
				throw rts2core::Error ("empty n parameter");
			rts2core::Value *val = ((XmlRpcd *) getMasterApp())->getOwnValue (vn);
			if (val == NULL)
				throw rts2core::Error (std::string ("cannot find variable with name ") + vn);
			if (!(val->getValueExtType () == RTS2_VALUE_STAT && val->getValueBaseType () == RTS2_VALUE_DOUBLE))
				throw rts2core::Error ("value is not statistical double type");
			((rts2core::ValueDoubleStat *) val)->clearStat ();
			((XmlRpcd *)getMasterApp ())->sendValueAll (val);
			sendOwnValues (os, params, NAN, false);
		}
		// set multiple values
		else if (vals[0] == "mset")
		{
			int async = params->getInteger ("async", 0);
			int ext = params->getInteger ("e", 0);

			int vcc = 0;
			int own_calls = 0;

			AsyncAPIMSet *aa = NULL;

			for (HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
			{
				// find device.name pairs..
				std::string dn (iter->getName ());
				size_t dot = dn.find ('.');
				if (dot != std::string::npos)
				{
					std::string devn = dn.substr (0,dot);
					std::string vn = dn.substr (dot + 1);
					if (devn == ((XmlRpcd *) getMasterApp ())->getDeviceName ())
					{
						((XmlRpcd *) getMasterApp ())->doOpValue (vn.c_str (), '=', iter->getValue ());
						own_calls++;
					}
					else
					{
						if (isCentraldName (devn.c_str ()))
							conn = master->getSingleCentralConn ();
						else
							conn = master->getOpenConnection (devn.c_str ());
						if (conn == NULL)
							throw JSONException ("cannot find device with name " + devn);
						rts2core::Value * rts2v = master->getValue (devn.c_str (), vn.c_str ());
						if (rts2v == NULL)
							throw JSONException ("cannot find variable with name " + vn);
						if (async == 0)
						{
							if (aa == NULL)
							{
								aa = new AsyncAPIMSet (this, conn, connection, ext);
								((XmlRpcd *) getMasterApp ())->registerAPI (aa);
							}
							aa->incCalls ();

						}

						conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), vn, '=', std::string (iter->getValue ()), true), 0, aa);
						vcc++;
					}
				}
			}
			if (async)
			{
				os << "\"ret\":0,\"value_changed\":" << vcc;
			}
			else
			{
				if (vcc > 0)
				{
					aa->incSucc (own_calls);
					throw XmlRpc::XmlRpcAsynchronous ();
				}
				os << "\"succ\":" << own_calls << ",\"failed\":0,\"ret\":0";
			}
		}
		// return night start and end
		else if (vals[0] == "night")
		{
			double ns = params->getDouble ("day", time (NULL));
			time_t ns_t = ns;
			Rts2Night n (ln_get_julian_from_timet (&ns_t), Configuration::instance ()->getObserver ());
			os << "\"from\":" << *(n.getFrom ()) << ",\"to\":" << *(n.getTo ());
		}
		// get variables from all connected devices
		else if (vals[0] == "getall")
		{
			bool ext = params->getInteger ("e", 0);
			double from = params->getDouble ("from", 0);

			// send own values first
			os << '"' << ((XmlRpcd *) getMasterApp ())->getDeviceName () << "\":{";
			sendOwnValues (os, params, from, ext);
			os << '}';

			for (rts2core::connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
			{
				if ((*iter)->getName ()[0] == '\0')
					continue;
				os << ",\"" << (*iter)->getName () << "\":{";
				sendConnectionValues (os, *iter, params, from, ext);
				os << '}';
			}
		}
		// get variables
		else if (vals[0] == "get" || vals[0] == "status")
		{
			const char *device = params->getString ("d","");
			bool ext = params->getInteger ("e", 0);
			double from = params->getDouble ("from", 0);
			if (strcmp (device, ((XmlRpcd *) getMasterApp ())->getDeviceName ()))
			{
				if (isCentraldName (device))
					conn = master->getSingleCentralConn ();
				else
					conn = master->getOpenConnection (device);
				if (conn == NULL)
					throw JSONException ("cannot find device");
				sendConnectionValues (os, conn, params, from, ext);
			}
			else
			{
				sendOwnValues (os, params, from, ext);
			}
		}
		else if (vals[0] == "push")
		{
			AsyncValueAPI *aa = new AsyncValueAPI (this, connection, params);
			aa->sendAll ((rts2core::Device *) getMasterApp ());
			((XmlRpcd *) getMasterApp ())->registerAPI (aa);

			throw XmlRpc::XmlRpcAsynchronous ();
		}
		else if (vals[0] == "simulate")
		{
			AsyncSimulateAPI *aa = new AsyncSimulateAPI (this, connection, params);
			((XmlRpcd *) getMasterApp ())->registerAPI (aa);

			throw XmlRpc::XmlRpcAsynchronous ();
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
			if (strcmp (device, ((XmlRpcd *) getMasterApp ())->getDeviceName ()))
			{
				if (isCentraldName (device))
					conn = master->getSingleCentralConn ();
				else
					conn = master->getOpenConnection (device);
				if (conn == NULL)
					throw JSONException ("cannot find device");
				if (async)
				{
					conn->queCommand (new rts2core::Command (master, cmd));
					sendConnectionValues (os, conn, params, NAN, ext);
				}
				else
				{
					AsyncAPI *aa = new AsyncAPI (this, conn, connection, ext);
					((XmlRpcd *) getMasterApp ())->registerAPI (aa);
	
					conn->queCommand (new rts2core::Command (master, cmd), 0, aa);
					throw XmlRpc::XmlRpcAsynchronous ();
				}
			}
			else
			{
				if (!strcmp (cmd, "autosave"))
					((XmlRpcd *) getMasterApp ())->autosaveValues ();
				else
					throw JSONException ("invalid command");
				sendOwnValues (os, params, NAN, ext);
			}
		}
		// start exposure, return from server image
		else if (vals[0] == "expose" || vals[0] == "exposedata")
		{
			const char *camera;
			long smin, smax;
			rts2image::scaling_type scaling;
			int newType;
			getCameraParameters (params, camera, smin, smax, scaling, newType);
			bool ext = params->getInteger ("e", 0);
			conn = master->getOpenConnection (camera);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");

			// this is safe, as all DEVICE_TYPE_CCDs are XmlDevCameraClient classes
			XmlDevCameraClient *camdev = (XmlDevCameraClient *) conn->getOtherDevClient ();

			camdev->setExpandPath (params->getString ("fe", camdev->getDefaultFilename ()));

			AsyncAPI *aa;
			if (vals[0] == "expose")
			{
				aa = new AsyncAPI (this, conn, connection, ext);
			}
			else
			{
				int chan = params->getInteger ("chan", 0);
				aa = new AsyncExposeAPI (this, conn, connection, chan, smin, smax, scaling, newType);
			}
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
			rts2image::Image *image = ((XmlDevCameraClient *) (conn->getOtherDevClient ()))->getPreviousImage ();
			os << "\"hasimage\":" << ((image == NULL) ? "false" : "true");
		}
		else if (vals[0] == "expand")
		{
			const char *fn = params->getString ("fn", NULL);
			if (!fn)
				throw JSONException ("empty image name");
			const char *e = params->getString ("e", NULL);
			if (!e)
				throw JSONException ("empty expand parameter");
			rts2image::Image image;
			image.openFile (fn, true, false);

			os << "\"expanded\":\"" << image.expandPath (std::string (e), false) << "\"";
		}
		else if (vals[0] == "runscript")
		{
			const char *d = params->getString ("d","");
			conn = master->getOpenConnection (d);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");
			bool callScriptEnd = true;
			const char *script = params->getString ("s","");
			if (strlen (script) == 0)
			{
				script = params->getString ("S","");
				if (strlen (script) == 0)
					throw JSONException ("you have to specify script (with s or S parameter)");
				callScriptEnd = false;
			}
			bool kills = params->getInteger ("kill", 0);

			XmlDevCameraClient *camdev = (XmlDevCameraClient *) conn->getOtherDevClient ();

			camdev->setCallScriptEnds (callScriptEnd);
			camdev->setScriptExpand (params->getString ("fe", camdev->getDefaultFilename ()));

			camdev->executeScript (script, kills);
			os << "\"d\":\"" << d << "\",\"s\":\"" << script << "\"";
		}
		else if (vals[0] == "killscript")
		{
			const char *d = params->getString ("d","");
			conn = master->getOpenConnection (d);
			if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
				throw JSONException ("cannot find camera with given name");

			XmlDevCameraClient *camdev = (XmlDevCameraClient *) conn->getOtherDevClient ();

			camdev->killScript ();
			os << "\"d\":\"" << d << "\",\"s\":\"\"";
		}
#ifdef RTS2_HAVE_PGSQL
		// returns target information specified by target name
		else if (vals[0] == "tbyname")
		{
			rts2db::TargetSet tar_set;
			const char *name = params->getString ("n", "");
			bool ic = params->getInteger ("ic",1);
			bool pm = params->getInteger ("pm",1);
			bool chunked = params->getInteger ("ch", 0);
			if (name[0] == '\0')
				throw JSONException ("empty n parameter");
			tar_set.loadByName (name, pm, ic);
			if (chunked)
			{
				sendAsyncDataHeader (0, connection, "application/json");
				jsonTargets (tar_set, os, params, NULL, connection);
				connection->asyncFinished ();
				return;
			}
			else
			{
				jsonTargets (tar_set, os, params);
			}

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
				tar_set.loadByName ("%", false, false);
			else	
				tar_set.loadByLabelId (label);
			jsonTargets (tar_set, os, params);
		}
		// returns targets within certain radius from given ra dec
		else if (vals[0] == "tbydistance")
		{
			struct ln_equ_posn pos;
			pos.ra = params->getDouble ("ra", NAN);
			pos.dec = params->getDouble ("dec", NAN);
			double radius = params->getDouble ("radius", NAN);
			if (isnan (pos.ra) || isnan (pos.dec) || isnan (radius))
				throw JSONException ("invalid ra, dec or radius parameter");
			rts2db::TargetSet ts (&pos, radius, Configuration::instance ()->getObserver ());
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
				rts2db::TargetSet ts (&pos, nearest, Configuration::instance ()->getObserver ());
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
			int t = params->getInteger ("t", -1);
			if (label[0] == '\0' && t < 0)
			{

			}
			if (label[0] == '\0')
				throw JSONException ("empty l parameter");
			if (t < 0)
				throw JSONException ("invalid type parametr");
			rts2db::Labels lb;
			os << "\"lid\":" << lb.getLabel (label, t);
		}
		else if (vals[0] == "consts")
		{
			rts2db::Target *target = getTarget (params);
			target->getConstraints ()->printJSON (os);
		}
		// violated constrainsts..
		else if (vals[0] == "violated")
		{
			const char *cn = params->getString ("consts", "");
			if (cn[0] == '\0')
				throw JSONException ("unknow constraint name");
			rts2db::Target *tar = getTarget (params);
			double from = params->getDouble ("from", getNow ());
			double to = params->getDouble ("to", from + 86400);
			// 60 sec = 1 minute step (by default)
			double step = params->getDouble ("step", 60);

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
			rts2db::Target *tar = getTarget (params);
			time_t from = params->getDouble ("from", getNow ());
			time_t to = params->getDouble ("to", from + 86400);
			double length = params->getDouble ("length", 1800);
			int step = params->getInteger ("step", 60);

			rts2db::interval_arr_t si;
			from -= from % step;
			to += step - (to % step);
			tar->getSatisfiedIntervals (from, to, length, step, si);
			os << "\"id\":" << tar->getTargetID () << ",\"satisfied\":[";
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
			rts2db::Target *tar = getTarget (params);

			std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > ac;

			if (vals[0] == "cnst_alt")
				tar->getAltitudeConstraints (ac);
			else
				tar->getAltitudeViolatedConstraints (ac);

			os << "\"id\":" << tar->getTargetID () << ",\"altitudes\":{";

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
					os << rts2json::JsonDouble (di->getLower ()) << "," << rts2json::JsonDouble (di->getUpper ()) << "]";
				}
				os << "]";
			}
			os << "}";
		}
		// return intervals of time constraints (or violated time intervals)
		else if (vals[0] == "cnst_time" || vals[0] == "cnst_time_v")
		{
			rts2db::Target *tar = getTarget (params);
			time_t from = params->getInteger ("from", getNow ());
			time_t to = params->getInteger ("to", from + 86400);
			double steps = params->getDouble ("steps", 1000);

			steps = double ((to - from)) / steps;

			std::map <std::string, rts2db::ConstraintPtr> cons;

			tar->getTimeConstraints (cons);

			os << "\"id\":" << tar->getTargetID () << ",\"constraints\":{";

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
			double ra = params->getDouble ("ra", NAN);
			double dec = params->getDouble ("dec", NAN);
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
			rts2db::Target *t = getTarget (params);
			switch (t->getTargetType ())
			{
				case TYPE_OPORTUNITY:
				case TYPE_GRB:
				case TYPE_FLAT:
				case TYPE_CALIBRATION:
				case TYPE_GPS:
				case TYPE_TERESTIAL:
				case TYPE_AUGER:
				case TYPE_LANDOLT:
				{
					rts2db::ConstTarget *tar = (rts2db::ConstTarget *) t;
					const char *tn = params->getString ("tn", "");
					double ra = params->getDouble ("ra", -1000);
					double dec = params->getDouble ("dec", -1000);
					double pm_ra = params->getDouble ("pm_ra", NAN);
					double pm_dec = params->getDouble ("pm_dec", NAN);
					bool enabled = params->getInteger ("enabled", tar->getTargetEnabled ());
					const char *desc = params->getString ("desc", NULL);

					if (strlen (tn) > 0)
						tar->setTargetName (tn);
					if (ra > -1000 && dec > -1000)
						tar->setPosition (ra, dec);
					if (!(isnan (pm_ra) && isnan (pm_dec)))
					{
						switch (tar->getTargetType ())
						{
							case TYPE_CALIBRATION:
							case TYPE_OPORTUNITY:
								((rts2db::ConstTarget *) (tar))->setProperMotion (pm_ra, pm_dec);
								break;
							default:
								throw JSONException ("only calibration and oportunity targets can have proper motion");
						}
					}
					tar->setTargetEnabled (enabled, true);
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
			rts2db::Target *tar = getTarget (params);
			const char *cam = params->getString ("c", NULL);
			if (strlen (cam) == 0)
				throw JSONException ("unknow camera");
			const char *s = params->getString ("s", "");
			if (strlen (s) == 0)
				throw JSONException ("empty script");
			tar->setScript (cam, s);
			os << "\"id\":" << tar->getTargetID () << ",\"camera\":\"" << cam << "\",\"script\":\"" << s << "\"";
			delete tar;
		}
		else if (vals[0] == "change_constraints")
		{
			rts2db::Target *tar = getTarget (params);
			const char *cn = params->getString ("cn", NULL);
			const char *ci = params->getString ("ci", NULL);
			if (cn == NULL)
				throw JSONException ("constraint not specified");
			rts2db::Constraints constraints;
			constraints.parse (cn, ci);
			tar->appendConstraints (constraints);

			os << "\"id\":" << tar->getTargetID () << ",";
			constraints.printJSON (os);

			delete tar;
		}
		else if (vals[0] == "tlabs_list")
		{
			rts2db::Target *tar = getTarget (params);
			jsonLabels (tar, os);
		}
		else if (vals[0] == "tlabs_delete")
		{
			rts2db::Target *tar = getTarget (params);
			int ltype = params->getInteger ("ltype", -1);
			if (ltype < 0)
				throw JSONException ("unknow/missing label type");
			tar->deleteLabels (ltype);
			jsonLabels (tar, os);
		}
		else if (vals[0] == "tlabs_add" || vals[0] == "tlabs_set")
		{
			rts2db::Target *tar = getTarget (params);
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
		else if (vals[0] == "lastobs")
		{
			rts2db::Observation obs;

			int ret = obs.loadLastObservation ();
			if (ret)
				throw JSONException ("cannot find last observation");

			jsonObservation (&obs, os);
		}
		else if (vals[0] == "obyid")
		{
			int obs_id = params->getInteger ("id", -1);
			if (obs_id < 0)
				throw JSONException ("unknow observation ID");

			rts2db::Observation obs (obs_id);
			if (obs.load ())
				throw JSONException ("cannot load observation with given ID");

			jsonObservation (&obs, os);
		}
		else if (vals[0] == "plan")
		{
			rts2db::PlanSet ps (params->getDouble ("from", getNow ()), params->getDouble ("to", NAN));
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
			double to = params->getDouble ("to", getNow ());
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
		else if (vals[0] == "msgqueue")
		{
			os << std::fixed << "\"h\":["
				"{\"n\":\"Time\",\"t\":\"t\",\"c\":0},"
				"{\"n\":\"Component\",\"t\":\"s\",\"c\":1},"
				"{\"n\":\"Type\",\"t\":\"n\",\"c\":2},"
				"{\"n\":\"Text\",\"t\":\"s\",\"c\":3}],"
				"\"d\":[";

			for (std::deque <Message>::iterator iter = ((XmlRpcd *) getMasterApp ())->getMessages ().begin (); iter != ((XmlRpcd *) getMasterApp ())->getMessages ().end (); iter++)
			{
				if (iter != ((XmlRpcd *) getMasterApp ())->getMessages ().begin ())
					os << ",";
				os << "[" << iter->getMessageTime () << ",\"" << iter->getMessageOName () << "\"," << iter->getType () << ",\"" << iter->getMessageString () << "\"]";
			}
			os << "]";
		}
#ifdef RTS2_HAVE_PGSQL
		else if (vals[0] == "auger")
		{
			int a_id = params->getInteger ("id", -1);
			rts2db::TargetAuger ta (-1, Configuration::instance ()->getObserver (), 10);
			if (a_id < 0)
			{
				a_id = params->getInteger ("obs_id", -1);
				if (a_id < 0)
					throw JSONException ("invalid observation ID");
				ta.loadByOid (a_id);
			}
			else
			{
				ta.load (a_id);
			}

			os << "\"t3id\":" << ta.t3id 
				<< ",\"auger_date\":" << rts2json::JsonDouble (ta.auger_date)
				<< ",\"Eye\":" << ta.Eye
				<< ",\"Run\":" << ta.Run
				<< ",\"Event\":" << ta.Event
				<< ",\"AugerId\":\"" << ta.AugerId 
				<< "\",\"GPSSec\":" << ta.GPSSec
				<< ",\"GPSNSec\":" << ta.GPSNSec
				<< ",\"SDId\":" << ta.SDId
				<< ",\"NPix\":" << ta.NPix
	
				<< ",\"SDPTheta\":" << rts2json::JsonDouble (ta.SDPTheta)       /// Zenith angle of SDP normal vector (deg)
				<< ",\"SDPThetaErr\":" << rts2json::JsonDouble (ta.SDPThetaErr)    /// Uncertainty of SDPtheta
				<< ",\"SDPPhi\":" << rts2json::JsonDouble (ta.SDPPhi)         /// Azimuth angle of SDP normal vector (deg)
				<< ",\"SDPPhiErr\":" << rts2json::JsonDouble (ta.SDPPhiErr)      /// Uncertainty of SDPphi
				<< ",\"SDPChi2\":" << rts2json::JsonDouble (ta.SDPChi2)        /// Chi^2 of SDP db_it
				<< ",\"SDPNdf\":" << ta.SDPNdf         /// Degrees of db_reedom of SDP db_it
	
				<< ",\"Rp\":" << rts2json::JsonDouble (ta.Rp)             /// Shower impact parameter Rp (m)
				<< ",\"RpErr\":" << rts2json::JsonDouble (ta.RpErr)          /// Uncertainty of Rp (m)
				<< ",\"Chi0\":" << rts2json::JsonDouble (ta.Chi0)           /// Angle of shower in the SDP (deg)
				<< ",\"Chi0Err\":" << rts2json::JsonDouble (ta.Chi0Err)        /// Uncertainty of Chi0 (deg)
				<< ",\"T0\":" << rts2json::JsonDouble (ta.T0)             /// FD time db_it T_0 (ns)
				<< ",\"T0Err\":" << rts2json::JsonDouble (ta.T0Err)          /// Uncertainty of T_0 (ns)
				<< ",\"TimeChi2\":" << rts2json::JsonDouble (ta.TimeChi2)       /// Full Chi^2 of axis db_it
				<< ",\"TimeChi2FD\":" << rts2json::JsonDouble (ta.TimeChi2FD)     /// Chi^2 of axis db_it (FD only)
				<< ",\"TimeNdf\":" << ta.TimeNdf        /// Degrees of db_reedom of axis db_it
	
				<< ",\"Easting\":" << rts2json::JsonDouble (ta.Easting)        /// Core position in easting coordinate (m)
				<< ",\"Northing\":" << rts2json::JsonDouble (ta.Northing)       /// Core position in northing coordinate (m)
				<< ",\"Altitude\":" << rts2json::JsonDouble (ta.Altitude)       /// Core position altitude (m)
				<< ",\"NorthingErr\":" << rts2json::JsonDouble (ta.NorthingErr)    /// Uncertainty of northing coordinate (m)
				<< ",\"EastingErr\":" << rts2json::JsonDouble (ta.EastingErr)     /// Uncertainty of easting coordinate (m)
				<< ",\"Theta\":" << rts2json::JsonDouble (ta.Theta)          /// Shower zenith angle in core coords. (deg)
				<< ",\"ThetaErr\":" << rts2json::JsonDouble (ta.ThetaErr)       /// Uncertainty of zenith angle (deg)
				<< ",\"Phi\":" << rts2json::JsonDouble (ta.Phi)            /// Shower azimuth angle in core coords. (deg)
				<< ",\"PhiErr\":" << rts2json::JsonDouble (ta.PhiErr)         /// Uncertainty of azimuth angle (deg)
	
				<< ",\"dEdXmax\":" << rts2json::JsonDouble (ta.dEdXmax)        /// Energy deposit at shower max (GeV/(g/cm^2))
				<< ",\"dEdXmaxErr\":" << rts2json::JsonDouble (ta.dEdXmaxErr)     /// Uncertainty of Nmax (GeV/(g/cm^2))
				<< ",\"Xmax\":" << rts2json::JsonDouble (ta.Xmax)           /// Slant depth of shower maximum (g/cm^2)
				<< ",\"XmaxErr\":" << rts2json::JsonDouble (ta.XmaxErr)        /// Uncertainty of Xmax (g/cm^2)
				<< ",\"X0\":" << rts2json::JsonDouble (ta.X0)             /// X0 Gaisser-Hillas db_it (g/cm^2)
				<< ",\"X0Err\":" << rts2json::JsonDouble (ta.X0Err)          /// Uncertainty of X0 (g/cm^2)
				<< ",\"Lambda\":" << rts2json::JsonDouble (ta.Lambda)         /// Lambda of Gaisser-Hillas db_it (g/cm^2)
				<< ",\"LambdaErr\":" << rts2json::JsonDouble (ta.LambdaErr)      /// Uncertainty of Lambda (g/cm^2)
				<< ",\"GHChi2\":" << rts2json::JsonDouble (ta.GHChi2)         /// Chi^2 of Gaisser-Hillas db_it
				<< ",\"GHNdf\":" << ta.GHNdf          /// Degrees of db_reedom of GH db_it
				<< ",\"LineFitChi2\":" << rts2json::JsonDouble (ta.LineFitChi2)    /// Chi^2 of linear db_it to profile
	
				<< ",\"EmEnergy\":" << rts2json::JsonDouble (ta.EmEnergy)       /// Calorimetric energy db_rom GH db_it (EeV)
				<< ",\"EmEnergyErr\":" << rts2json::JsonDouble (ta.EmEnergyErr)    /// Uncertainty of Eem (EeV)
				<< ",\"Energy\":" << rts2json::JsonDouble (ta.Energy)         /// Total energy db_rom GH db_it (EeV)
				<< ",\"EnergyErr\":" << rts2json::JsonDouble (ta.EnergyErr)      /// Uncertainty of Etot (EeV)
	
				<< ",\"MinAngle\":" << rts2json::JsonDouble (ta.MinAngle)       /// Minimum viewing angle (deg)
				<< ",\"MaxAngle\":" << rts2json::JsonDouble (ta.MaxAngle)       /// Maximum viewing angle (deg)
				<< ",\"MeanAngle\":" << rts2json::JsonDouble (ta.MeanAngle)      /// Mean viewing angle (deg)
		
				<< ",\"NTank\":" << ta.NTank          /// Number of stations in hybrid db_it
				<< ",\"HottestTank\":" << ta.HottestTank    /// Station used in hybrid-geometry reco
				<< ",\"AxisDist\":" << rts2json::JsonDouble (ta.AxisDist)       /// Shower axis distance to hottest station (m)
				<< ",\"SDPDist\":" << rts2json::JsonDouble (ta.SDPDist)        /// SDP distance to hottest station (m)
	
				<< ",\"SDFDdT\":" << rts2json::JsonDouble (ta.SDFDdT)         /// SD/FD time offset after the minimization (ns)
				<< ",\"XmaxEyeDist\":" << rts2json::JsonDouble (ta.XmaxEyeDist)    /// Distance to shower maximum (m)
				<< ",\"XTrackMin\":" << rts2json::JsonDouble (ta.XTrackMin)      /// First recorded slant depth of track (g/cm^2)
				<< ",\"XTrackMax\":" << rts2json::JsonDouble (ta.XTrackMax)      /// Last recorded slant depth of track (g/cm^2)
				<< ",\"XFOVMin\":" << rts2json::JsonDouble (ta.XFOVMin)        /// First slant depth inside FOV (g/cm^2)
				<< ",\"XFOVMax\":" << rts2json::JsonDouble (ta.XFOVMax)        /// Last slant depth inside FOV (g/cm^2)
				<< ",\"XTrackObs\":" << rts2json::JsonDouble (ta.XTrackObs)      /// Observed track length depth (g/cm^2)
				<< ",\"DegTrackObs\":" << rts2json::JsonDouble (ta.DegTrackObs)    /// Observed track length angle (deg)
				<< ",\"TTrackObs\":" << rts2json::JsonDouble (ta.TTrackObs)      /// Observed track length time (100 ns)

				<< ",\"cut\":" << ta.cut               /// Cuts pased by shower

				<< ",\"profile\":[";

			for (std::vector <std::pair <double, double> >::iterator iter = ta.showerparams.begin (); iter != ta.showerparams.end (); iter++)
			{
				if (iter != ta.showerparams.begin ())
					os << ",";
				os << "[" << iter->first << "," << iter->second << "]";
			}
			os << "]";
		}
#endif // RTS2_HAVE_PGSQL
		else
		{
			throw JSONException ("invalid request " + path);
		}
		os << "}";
	}

	returnJSON (os.str ().c_str (), response_type, response, response_length);
}

void API::sendConnectionValues (std::ostringstream & os, rts2core::Connection * conn, HttpParams *params, double from, bool extended)
{
	os << "\"d\":{" << std::fixed;
	double mfrom = NAN;
	bool first = true;
	rts2core::ValueVector::iterator iter;

	for (iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		if ((isnan (from) || from > 0) && conn->getOtherDevClient ())
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

		jsonValue (*iter, extended, os);
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
			os << "\"" << v->getName () << "\":[" << rts2json::JsonDouble (v->getMin ()) << "," << rts2json::JsonDouble (v->getMax ()) << "]";
		}
	}

	os << "},\"idle\":" << conn->isIdle () << ",\"state\":" << conn->getState () << ",\"sstart\":" << rts2json::JsonDouble (conn->getProgressStart ()) << ",\"send\":" << rts2json::JsonDouble (conn->getProgressEnd ()) << ",\"f\":" << rts2json::JsonDouble (mfrom);
}

void API::sendOwnValues (std::ostringstream & os, HttpParams *params, double from, bool extended)
{
	os << "\"d\":{" << std::fixed;
	bool first = true;

	XmlRpcd *master = (XmlRpcd *) getMasterApp ();
	rts2core::CondValueVector::iterator iter;

	for (iter = master->getValuesBegin (); iter != master->getValuesEnd (); iter++)
	{
		if (first)
			first = false;
		else
			os << ",";

		jsonValue ((*iter)->getValue (), extended, os);
	}
	os << "},\"minmax\":{";

	bool firstMMax = true;

	for (iter = master->getValuesBegin (); iter != master->getValuesEnd (); iter++)
	{
		rts2core::Value *val = (*iter)->getValue ();
		if (val->getValueExtType () == RTS2_VALUE_MMAX && val->getValueBaseType () == RTS2_VALUE_DOUBLE)
		{
			rts2core::ValueDoubleMinMax *v = (rts2core::ValueDoubleMinMax *) (val);
			if (firstMMax)
				firstMMax = false;
			else
				os << ",";
			os << "\"" << v->getName () << "\":[" << rts2json::JsonDouble (v->getMin ()) << "," << rts2json::JsonDouble (v->getMax ()) << "]";
		}
	}

	os << "},\"idle\":" << ((master->getState () & DEVICE_STATUS_MASK) == DEVICE_IDLE) << ",\"state\":" << master->getState () << ",\"f\":" << rts2json::JsonDouble (from); 
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

#ifdef RTS2_HAVE_PGSQL
void API::jsonTargets (rts2db::TargetSet &tar_set, std::ostringstream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom, XmlRpc::XmlRpcServerConnection * chunked)
{
	bool extended = params->getInteger ("e", false);
	bool withpm = params->getInteger ("propm", false);
	time_t from = params->getInteger ("from", getNow ());
	int c = 5;
	if (chunked)
		os << "\"rows\":" << tar_set.size () << ",";

	os << "\"h\":["
		"{\"n\":\"Target ID\",\"t\":\"n\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":0},"
		"{\"n\":\"Target Name\",\"t\":\"a\",\"prefix\":\"" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/\",\"href\":0,\"c\":1},"
		"{\"n\":\"RA\",\"t\":\"r\",\"c\":2},"
		"{\"n\":\"DEC\",\"t\":\"d\",\"c\":3},"
		"{\"n\":\"Description\",\"t\":\"s\",\"c\":4}";


	struct ln_equ_posn oradec;

	if (dfrom == NULL)
	{
			oradec.ra = params->getDouble ("ra", NAN);
			oradec.dec = params->getDouble ("dec", NAN);
			if (!isnan (oradec.ra) && !isnan (oradec.dec))
				dfrom = &oradec;
	}
	if (dfrom)
	{
		os << ",{\"n\":\"Distance\",\"t\":\"d\",\"c\":" << (c) << "}";
		c++;
	}
	if (extended)
	{
		os << ",{\"n\":\"Duration\",\"t\":\"dur\",\"c\":" << (c) << "},"
		"{\"n\":\"Scripts\",\"t\":\"scripts\",\"c\":" << (c + 1) << "},"
		"{\"n\":\"Satisfied\",\"t\":\"s\",\"c\":" << (c + 2) << "},"
		"{\"n\":\"Violated\",\"t\":\"s\",\"c\":" << (c + 3) << "},"
		"{\"n\":\"Transit\",\"t\":\"t\",\"c\":" << (c + 4) << "},"
		"{\"n\":\"Observable\",\"t\":\"t\",\"c\":" << (c + 5) << "}";
		c += 6;
	}
	if (withpm)
	{
		os << ",{\"n\":\"Proper motion RA\",\"t\":\"d\",\"c\":" << (c) << "},"
		"{\"n\":\"Proper motion DEC\",\"t\":\"d\",\"c\":" << (c + 1) << "}";
		c += 2;
	}

	if (chunked)
	{
		os << "]}";
		chunked->sendChunked (os.str ());
		os.str ("");
		os << std::fixed;
	}
	else
	{
		os << "],\"d\":[" << std::fixed;
	}

	double JD = ln_get_julian_from_timet (&from);
	for (rts2db::TargetSet::iterator iter = tar_set.begin (); iter != tar_set.end (); iter++)
	{
		if (iter != tar_set.begin () && chunked == NULL)
			os << ",";
		struct ln_equ_posn equ;
		rts2db::Target *tar = iter->second;
		tar->getPosition (&equ, JD);
		const char *n = tar->getTargetName ();
		os << "[" << tar->getTargetID () << ",";
		if (n == NULL)
			os << "null,";
		else
			os << "\"" << rts2json::JsonString (n) << "\",";

		os << rts2json::JsonDouble (equ.ra) << ',' << rts2json::JsonDouble (equ.dec) << ",\"" << rts2json::JsonString (tar->getTargetInfo ()) << "\"";

		if (dfrom)
			os << ',' << rts2json::JsonDouble (tar->getDistance (dfrom, JD));

		if (extended)
		{
			double md = -1;
			std::ostringstream cs;
			for (rts2db::CamList::iterator cam = ((XmlRpcd *) getMasterApp ())->cameras.begin (); cam != ((XmlRpcd *) getMasterApp ())->cameras.end (); cam++)
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

				os << "," << rts2json::JsonDouble (rst.transit) 
					<< "," << (tar->isAboveHorizon (&hrz) ? "true" : "false");
			}
		}
		if (withpm)
		{
			struct ln_equ_posn pm;
			switch (tar->getTargetType ())
			{
				case TYPE_CALIBRATION:
				case TYPE_OPORTUNITY:
					((rts2db::ConstTarget *) tar)->getProperMotion (&pm);
					break;
				default:
					pm.ra = pm.dec = NAN;
			}

			os << "," << rts2json::JsonDouble (pm.ra) << "," << rts2json::JsonDouble (pm.dec);
		}
		os << "]";
		if (chunked)
		{
			chunked->sendChunked (os.str ());
			os.str ("");
		}
	}
	if (chunked == NULL)
	{
		os << "]";
	}	
}

void jsonObservation (rts2db::Observation *obs, std::ostream &os)
{
	struct ln_equ_posn equ;
	obs->getTarget ()->getPosition (&equ);

	os << std::fixed << "\"observation\":{\"id\":" << obs->getObsId ()
		<< ",\"slew\":" << rts2json::JsonDouble (obs->getObsSlew ())
		<< ",\"start\":" << rts2json::JsonDouble (obs->getObsStart ())
		<< ",\"end\":" << rts2json::JsonDouble (obs->getObsEnd ())
		<< ",\"images\":" << obs->getNumberOfImages ()
		<< ",\"good\":" << obs->getNumberOfGoodImages ()
		<< "},\"target\":{\"id\":" << obs->getTarget ()->getTargetID ()
		<< ",\"name\":\"" << rts2json::JsonString (obs->getTarget ()->getTargetName ())
		<< "\",\"ra\":" << rts2json::JsonDouble (equ.ra)
		<< ",\"dec\":" << rts2json::JsonDouble (equ.dec)
		<< ",\"description\":\"" << rts2json::JsonString (obs->getTarget ()->getTargetInfo ())
		<< "\"}";
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
			<< rts2json::JsonDouble(iter->getObsSlew ()) << ","
			<< rts2json::JsonDouble(iter->getObsStart ()) << ","
			<< rts2json::JsonDouble(iter->getObsEnd ()) << ","
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
			<< rts2json::JsonDouble ((*iter)->getExposureSec () + (*iter)->getExposureUsec () / USEC_SEC) << ","
			<< rts2json::JsonDouble ((*iter)->getExposureLength ()) << ",\""
			<< (*iter)->getFilter () << "\",\""
			<< (*iter)->getAbsoluteFileName () << "\"]";
	}

	os << "]";
}

void API::jsonLabels (rts2db::Target *tar, std::ostream &os)
{
	rts2db::LabelsVector tlabels = tar->getLabels ();
	os << "\"id\":" << tar->getTargetID () << ",\"labels\":[";
	for (rts2db::LabelsVector::iterator iter = tlabels.begin (); iter != tlabels.end (); iter++)
	{
		if (iter == tlabels.begin ())
			os << "[";
		else
			os << ",[";
		os << iter->lid << "," << iter->ltype << ",\"" << iter->ltext << "\"]";
	}
	os << "]";
}
#endif // RTS2_HAVE_PGSQL

#ifdef RTS2_HAVE_PGSQL
rts2db::Target * API::getTarget (XmlRpc::HttpParams *params)
{
	int id = params->getInteger ("id", -1);
	if (id <= 0)
		throw JSONException ("invalid id parameter");
	rts2db::Target *target = createTarget (id, Configuration::instance ()->getObserver (), ((XmlRpcd *) getMasterApp ())->getNotifyConnection ());
	if (target == NULL)
		throw JSONException ("cannot find target with given ID");
	return target;
}
#endif // RTS2_HAVE_PGSQL
