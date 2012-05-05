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
#include "rts2db/imageset.h"
#include "rts2db/observationset.h"

/** @file api.h
 *
 * This header file declares classes for support of various JSON API calls. It
 * also includes documentation of those API calls.
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
class AsyncAPI:public rts2core::Object
{
	public:
		AsyncAPI (API *_req, rts2core::Connection *_conn, XmlRpcServerConnection *_source, bool _ext);
		virtual ~AsyncAPI ();
		
		virtual void postEvent (rts2core::Event *event);

		virtual void newDataConn (rts2core::Connection *_conn, int data_conn) {}
		virtual void dataReceived (rts2core::Connection *_conn, rts2core::DataAbstractRead *data) {}
		virtual void fullDataReceived (rts2core::Connection *_conn, rts2core::DataChannels *data) {}
		virtual void exposureFailed (rts2core::Connection *_conn, int status) {}
		virtual void exposureEnd (rts2core::Connection *_conn) {}

		/**
		 * Check if the request is for connection or source..
		 */
		bool isForSource (XmlRpcServerConnection *_source) { return source == _source; }
		bool isForConnection (rts2core::Connection *_conn) { return conn == _conn; }

		/**
		 * Null XMLRPC source. This also marks source for deletion.
		 */
		virtual void nullSource () { source = NULL; }

		void asyncFinished ()
		{
			if (source)
				source->asyncFinished ();
			nullSource ();
		}

		/**
		 * Check if the connection can be deleted, check for 
		 * shared memory segments.
		 *
		 * @return 0 if AsyncAPI is still active, otherwise AsyncAPI will be deleted
		 */
		virtual int idle () { return source == NULL; }

	protected:
		API *req;
		XmlRpcServerConnection *source;
		rts2core::Connection *conn;

	private:
		bool ext;
};

typedef enum { SCALING_LINEAR, SCALING_LOG } scaling_type;

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
		void sendConnectionValues (std::ostringstream &os, rts2core::Connection * conn, XmlRpc::HttpParams *params, double from = rts2_nan ("f"), bool extended = false);

		void sendOwnValues (std::ostringstream & os, HttpParams *params, double from, bool extended);
	private:
		void executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

#ifdef HAVE_PGSQL
		void jsonTargets (rts2db::TargetSet &tar_set, std::ostream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom = NULL);
		void jsonObservations (rts2db::ObservationSet *obss, std::ostream &os);
		void jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params);
		void jsonLabels (rts2db::Target *tar, std::ostream &os);
#endif
		void getCameraParameters (XmlRpc::HttpParams *params, const char *&camera, long &smin, long &smax, scaling_type &scaling);
#ifdef HAVE_PGSQL
		rts2db::Target * getTarget (XmlRpc::HttpParams *params);
#endif
};

}
