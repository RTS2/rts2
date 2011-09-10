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
		AsyncAPI (API *_req, Rts2Conn *_conn, XmlRpcServerConnection *_source);
		
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
		void sendConnectionValues (std::ostringstream &os, Rts2Conn * conn, XmlRpc::HttpParams *params, double from = rts2_nan ("f"));
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
		void jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params);
#endif
};

}
