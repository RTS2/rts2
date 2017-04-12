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
  "rts2-httpd" [shape=box];
  "RTS2 device" [shape=box];
  "SQL database" [shape=box];

  "Client" -> "HTTP/JSON library" [label="function call"];
  "HTTP/JSON library" -> "rts2-httpd" [label="HTTP GET request"];
  "rts2-httpd" -> "RTS2 device" [label="RTS2 protocol request call"];
  "rts2-httpd" -> "SQL database" [label="SQL call"];

  "rts2-httpd" -> "HTTP/JSON library" [label="JSON document with returned data"];
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

#include "httpd.h"
#include "rts2json/jsonvalue.h"

#include "rts2db/constraints.h"
#include "rts2db/planset.h"
#include "rts2script/script.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/labellist.h"
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

#ifdef RTS2_HAVE_PGSQL
API::API (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::JSONDBRequest (prefix, _http_server, s)
#else
API::API (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::JSONRequest (prefix, _http_server, s)
#endif
{
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

void API::executeJSON (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;
	rts2core::Connection *conn = NULL;

	HttpD * master = (HttpD*) getMasterApp ();

	// widgets - divs with informations
	if (vals.size () >= 2 && vals[0] == "w")
	{
		getWidgets (vals, params, response_type, response, response_length);
		return;
	}

	os.precision (8);

	// calls returning binary data
	if (vals.size () == 1)
	{
		if (vals[0] == "currentimage" || vals[0] == "lastimage")
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
				// HttpD::createOtherType qurantee that the other connection is XmlDevCameraClient
				// first try if there is data connection opened, so image data should be streamed in
				if (DataAbstractRead * lastData = conn->lastDataChannel (chan))
				{
					rts2json::AsyncCurrentAPI *aa = new rts2json::AsyncCurrentAPI (this, conn, connection, lastData, chan, smin, smax, scaling, newType);
					getServer ()->registerAPI (aa);

					throw XmlRpc::XmlRpcAsynchronous ();
				}
			}

			// HttpD::createOtherType qurantee that the other connection is XmlDevCameraClient
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
		else if (vals[0] == "devices")
		{
			int ext = params->getInteger ("e", 0);
			os << "[";
			if (ext)
				os << "[\"" << ((HttpD *) getMasterApp ())->getDeviceName () << "\"," << DEVICE_TYPE_HTTPD << ']';
			else
				os << '"' << ((HttpD *) getMasterApp ())->getDeviceName () << '"';
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
		else if (vals[0] == "devbytype")
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
		else if (vals[0] == "selval")
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
		else if (vals[0] == "sunalt")
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
		else if (vals[0] == "script")
		{
			rts2db::Target *target = rts2json::getTarget (params);
			const char *cname = params->getString ("cn", "");
			if (cname[0] == '\0')
				throw JSONException ("empty camera name");
			
			rts2script::Script script = rts2script::Script ();
			script.setTarget (cname, target);
			script.prettyPrint (os, rts2script::PRINT_JSON);
		}
		// return altitude of target..
		else if (vals[0] == "taltitudes")
		{
			time_t from = params->getInteger ("from", getNow () - 86400);
			time_t to = params->getInteger ("to", from + 86400);
			const int steps = params->getInteger ("steps", 1000);

			rts2db::Target *target = rts2json::getTarget (params);

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
		else
		{
		// returning JSON data
			os << "{";
			if (vals[0] == "executor")
			{
				connections_t::iterator iter = master->getConnections ()->begin ();
				master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
				if (iter == master->getConnections ()->end ())
				 	throw JSONException ("executor is not connected");
				rts2json::sendConnectionValues (os, *iter, params);
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
				os << ",\"readonly\":" << (canWriteDevice (device) ? "false" : "true");
			}
			// set or increment variable
			else if (vals[0] == "set" || vals[0] == "inc" || vals[0] == "dec")
			{
				const char *device = params->getString ("d","");
				if (!canWriteDevice (std::string (device)))
					throw JSONException ("not authorized to write to the device");
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
				if (!(strcmp (device, ((HttpD *) getMasterApp ())->getDeviceName ())))
				{
					((HttpD *) getMasterApp ())->doOpValue (variable, op, value);
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
						rts2json::sendConnectionValues (os, conn, params, ext);
					}
					else
					{
						rts2json::AsyncAPI *aa = new rts2json::AsyncAPI (this, conn, connection, ext);
						getServer ()->registerAPI (aa);

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
				rts2core::Value *val = ((HttpD *) getMasterApp())->getOwnValue (vn);
				if (val == NULL)
					throw rts2core::Error (std::string ("cannot find variable with name ") + vn);
				if (!(val->getValueExtType () == RTS2_VALUE_STAT && val->getValueBaseType () == RTS2_VALUE_DOUBLE))
					throw rts2core::Error ("value is not statistical double type");
				((rts2core::ValueDoubleStat *) val)->addValue (v, num);
				((HttpD *)getMasterApp ())->sendValueAll (val);
				sendOwnValues (os, params, NAN, false);
			}
			else if (vals[0] == "statclear")
			{
				const char *vn = params->getString ("n","");
				if (strlen (vn) == 0)
					throw rts2core::Error ("empty n parameter");
				rts2core::Value *val = ((HttpD *) getMasterApp())->getOwnValue (vn);
				if (val == NULL)
					throw rts2core::Error (std::string ("cannot find variable with name ") + vn);
				if (!(val->getValueExtType () == RTS2_VALUE_STAT && val->getValueBaseType () == RTS2_VALUE_DOUBLE))
					throw rts2core::Error ("value is not statistical double type");
				((rts2core::ValueDoubleStat *) val)->clearStat ();
				((HttpD *)getMasterApp ())->sendValueAll (val);
				sendOwnValues (os, params, NAN, false);
			}
			// set multiple values
			else if (vals[0] == "mset")
			{
				int async = params->getInteger ("async", 0);
				int ext = params->getInteger ("e", 0);

				int vcc = 0;
				int own_calls = 0;

				rts2json::AsyncAPIMSet *aa = NULL;

				for (HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
				{
					// find device.name pairs..
					std::string dn (iter->getName ());
					size_t dot = dn.find ('.');
					if (dot != std::string::npos)
					{
						std::string devn = dn.substr (0,dot);
						std::string vn = dn.substr (dot + 1);
						if (!canWriteDevice (devn))
							throw JSONException ("not authorized to write to the device");
						if (devn == ((HttpD *) getMasterApp ())->getDeviceName ())
						{
							((HttpD *) getMasterApp ())->doOpValue (vn.c_str (), '=', iter->getValue ());
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
									aa = new rts2json::AsyncAPIMSet (this, conn, connection, ext);
									getServer ()->registerAPI (aa);
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

				// send centrald values
				os << "\"centrald\":{";
				rts2json::sendConnectionValues (os, master->getSingleCentralConn (), params, from, ext);

				// send own values first
				os << "},\"" << ((HttpD *) getMasterApp ())->getDeviceName () << "\":{";
				sendOwnValues (os, params, from, ext);
				os << '}';

				for (rts2core::connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
				{
					if ((*iter)->getName ()[0] == '\0')
						continue;
					os << ",\"" << (*iter)->getName () << "\":{";
					rts2json::sendConnectionValues (os, *iter, params, from, ext);
					os << '}';
				}
			}
			// get variables
			else if (vals[0] == "get" || vals[0] == "status")
			{
				const char *device = params->getString ("d","");
				bool ext = params->getInteger ("e", 0);
				double from = params->getDouble ("from", 0);
				if (strcmp (device, ((HttpD *) getMasterApp ())->getDeviceName ()))
				{
					if (isCentraldName (device))
						conn = master->getSingleCentralConn ();
					else
						conn = master->getOpenConnection (device);
					if (conn == NULL)
						throw JSONException ("cannot find device");
					rts2json::sendConnectionValues (os, conn, params, from, ext);
				}
				else
				{
					sendOwnValues (os, params, from, ext);
				}
			}
			else if (vals[0] == "push")
			{
				rts2json::AsyncValueAPI *aa = new rts2json::AsyncValueAPI (this, connection, params);
				aa->sendAll ((rts2core::Device *) getMasterApp ());
				getServer ()->registerAPI (aa);

				throw XmlRpc::XmlRpcAsynchronous ();
			}
			else if (vals[0] == "simulate")
			{
				rts2json::AsyncSimulateAPI *aa = new rts2json::AsyncSimulateAPI (this, connection, params);
				getServer ()->registerAPI (aa);

				throw XmlRpc::XmlRpcAsynchronous ();
			}
			// execute command on server
			else if (vals[0] == "cmd")
			{
				const char *device = params->getString ("d", "");
				if (!canWriteDevice (std::string (device)))
					throw JSONException ("not authorized to write to the device");
				const char *cmd = params->getString ("c", "");
				int async = params->getInteger ("async", 0);
				bool ext = params->getInteger ("e", 0);
				if (cmd[0] == '\0')
					throw JSONException ("empty command");
				if (strcmp (device, ((HttpD *) getMasterApp ())->getDeviceName ()))
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
						rts2json::sendConnectionValues (os, conn, params, NAN, ext);
					}
					else
					{
						rts2json::AsyncAPI *aa = new rts2json::AsyncAPI (this, conn, connection, ext);
						getServer ()->registerAPI (aa);
		
						conn->queCommand (new rts2core::Command (master, cmd), 0, aa);
						throw XmlRpc::XmlRpcAsynchronous ();
					}
				}
				else
				{
					if (!strcmp (cmd, "autosave"))
						((HttpD *) getMasterApp ())->autosaveValues ();
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
				camdev->setOverwrite (params->getBoolean ("overwrite", false));

				rts2json::AsyncAPI *aa;
				if (vals[0] == "expose")
				{
					aa = new rts2json::AsyncAPI (this, conn, connection, ext);
				}
				else
				{
					int chan = params->getInteger ("chan", 0);
					aa = new rts2json::AsyncExposeAPI (this, conn, connection, chan, smin, smax, scaling, newType);
				}
				getServer ()->registerAPI (aa);

				conn->queCommand (new rts2core::CommandExposure (master, camdev, 0), 0, aa);
				throw XmlRpc::XmlRpcAsynchronous ();
			}
			else if (vals[0] == "hasimage")
			{
				const char *camera = params->getString ("ccd","");
				conn = master->getOpenConnection (camera);
				if (conn == NULL || conn->getOtherType () != DEVICE_TYPE_CCD)
					throw JSONException ("cannot find camera with given name");
				// HttpD::createOtherType qurantee that the other connection is XmlDevCameraClient
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
			else if (vals[0] == "msgqueue")
			{
				os << std::fixed << "\"h\":["
					"{\"n\":\"Time\",\"t\":\"t\",\"c\":0},"
					"{\"n\":\"Component\",\"t\":\"s\",\"c\":1},"
					"{\"n\":\"Type\",\"t\":\"n\",\"c\":2},"
					"{\"n\":\"Text\",\"t\":\"s\",\"c\":3}],"
					"\"d\":[";

				for (std::deque <Message>::iterator iter = ((HttpD *) getMasterApp ())->getMessages ().begin (); iter != ((HttpD *) getMasterApp ())->getMessages ().end (); iter++)
				{
					if (iter != ((HttpD *) getMasterApp ())->getMessages ().begin ())
						os << ",";
					os << "[" << iter->getMessageTime () << ",\"" << iter->getMessageOName () << "\"," << iter->getType () << ",\"" << iter->getMessageString () << "\"]";
				}
				os << "]";
			}
			else
			{
#ifdef RTS2_HAVE_PGSQL
				dbJSON (vals, source, path, params, os);
#else
				throw JSONException ("invalid request " + path);
#endif // RTS2_HAVE_PGSQL
			}
			os << "}";
		}
	}

	returnJSON (os, response_type, response, response_length);
}

void API::sendOwnValues (std::ostringstream & os, HttpParams *params, double from, bool extended)
{
	os << "\"d\":{" << std::fixed;
	bool first = true;

	HttpD *master = (HttpD *) getMasterApp ();
	rts2core::CondValueVector::iterator iter;

	for (iter = master->getValuesBegin (); iter != master->getValuesEnd (); iter++)
	{
		if (first)
			first = false;
		else
			os << ",";

		rts2json::jsonValue ((*iter)->getValue (), extended, os);
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
