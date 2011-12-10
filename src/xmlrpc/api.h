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

#include "httpreq.h"
#include "block.h"
#include "../../lib/rts2db/imageset.h"
#include "../../lib/rts2db/observationset.h"

/** @file api.h
 *
 * This header file declares classes for support of various JSON API calls. It
 * also includes documentation of those API calls.
 *
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
  "rts2-xmlrpcd" -> "RTS2 device" [label="RTS2 protcol request call"];
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
 * @page JSON_sql JSON database API
 *
 * @section targets List available targets
 *
 */

namespace rts2xmlrpc
{

/**
 * Display double value as JSON - instead of nan, write null.
 */
class JsonDouble
{
	public:
		JsonDouble (double _v) { v = _v; }
		friend std::ostream & operator << (std::ostream &os, JsonDouble d)
		{
			if (isnan (d.v) || isinf (d.v))
				os << "null";
			else
				os << d.v;
			return os;
		}

	private:
		double v;
};

class API;

/**
 * Contain code exacuted when async command returns.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class AsyncAPI:public Rts2Object
{
	public:
		AsyncAPI (API *_req, Rts2Conn *_conn, XmlRpcServerConnection *_source, bool _ext);
		
		virtual void postEvent (Rts2Event *event);

		/**
		 * Check if the request is for connection or source..
		 */
		bool isForSource (XmlRpcServerConnection *_source) { return source == _source; }
		bool isForConnection (Rts2Conn *_conn) { return conn == _conn; }

	protected:
		API *req;
		XmlRpcServerConnection *source;

	private:
		Rts2Conn *conn;
		bool ext;
};

/**
 * Class for API requests.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class API:public GetRequestAuthorized
{
	public:
		API (const char* prefix, XmlRpc::XmlRpcServer* s);

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		/**
		 * Send connection values as JSON string to the client.
		 *
		 * @param time from which changed values will be reported. nan means that all values will be reported.
		 */
		void sendConnectionValues (std::ostringstream &os, Rts2Conn * conn, XmlRpc::HttpParams *params, double from = rts2_nan ("f"), bool extended = false);
	private:
		void executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

		void sendArrayValue (rts2core::Value *value, std::ostringstream &os);
		void sendStatValue (rts2core::Value *value, std::ostringstream &os);
		void sendRectangleValue (rts2core::Value *value, std::ostringstream &os);
		void sendValue (rts2core::Value *value, std::ostringstream &os);
		void sendSelection (std::ostringstream &os, rts2core::ValueSelection *value);
#ifdef HAVE_PGSQL
		void jsonTargets (rts2db::TargetSet &tar_set, std::ostream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom = NULL);
		void jsonObservations (rts2db::ObservationSet *obss, std::ostream &os);
		void jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params);
		void jsonLabels (rts2db::Target *tar, std::ostream &os);
#endif
};

}
