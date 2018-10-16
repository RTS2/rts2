/* 
 * Asynchronous API objects.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011-2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_ASYNCAPI__
#define __RTS2_ASYNCAPI__

#include "httpreq.h"
#include "rts2fits/image.h"
#include "xmlrpc++/XmlRpc.h"
#include "device.h"

namespace rts2json
{

/**
 * Contain code exacuted when async command returns.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class AsyncAPI:public rts2core::Object
{
	public:
		AsyncAPI (JSONRequest *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, bool _ext);
		virtual ~AsyncAPI ();
		
		virtual void postEvent (rts2core::Event *event);

		virtual void newDataConn (__attribute__ ((unused)) rts2core::Connection *_conn, __attribute__ ((unused)) int data_conn) {}
		virtual void dataReceived (__attribute__ ((unused)) rts2core::Connection *_conn, __attribute__ ((unused)) rts2core::DataAbstractRead *data) {}
		virtual void fullDataReceived (__attribute__ ((unused)) rts2core::Connection *_conn, __attribute__ ((unused)) rts2core::DataChannels *data) {}
		virtual void exposureFailed (__attribute__ ((unused)) rts2core::Connection *_conn, __attribute__ ((unused)) int status) {}
		virtual void exposureEnd (__attribute__ ((unused)) rts2core::Connection *_conn) {}

		virtual void stateChanged (__attribute__ ((unused)) rts2core::Connection *_conn) {};
		virtual void valueChanged (__attribute__ ((unused)) rts2core::Connection *_conn, __attribute__ ((unused)) rts2core::Value *_value) {};

		/**
		 * Check if the request is for connection or source..
		 */
		bool isForSource (XmlRpc::XmlRpcServerConnection *_source) { return source == _source; }
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
		JSONRequest *req;
		XmlRpc::XmlRpcServerConnection *source;
		rts2core::Connection *conn;

	private:
		bool ext;
};

/**
 * Auxiliary struct for state information. It is used to store state,
 * so state updates will be send on JSON just once.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class AsyncState
{
	public:
		AsyncState (const char *connection_name):name (connection_name)
		{
			value = 0xffffffff;
			status_start = status_expected_end = NAN;
		}

		std::string name;
		rts2_status_t value;
		double status_start;
		double status_expected_end;
};

/**
 * Asynchronous class for value and state changes. Used to handle the "push" method.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class AsyncValueAPI:public AsyncAPI
{
	public:
		AsyncValueAPI (JSONRequest *_req, XmlRpc::XmlRpcServerConnection *_source, XmlRpc::HttpParams *params);

		virtual void stateChanged (rts2core::Connection *_conn);

		virtual void valueChanged (rts2core::Connection *_conn, rts2core::Value *_value);
		/**
		 * Send all registered values and states on JSON connection. Throw an error if value/connection
		 * cannot be found.
		 */
		void sendAll (rts2core::Device *device);

	private:
		// values registered for ASYNC API
		std::list <AsyncState> states;
		std::vector <std::string> devices;
		std::vector <std::pair <std::string, std::string> > values;

		void sendState (std::list <AsyncState>::iterator astate, rts2core::Connection *_conn);
		void sendValue (const std::string &device, rts2core::Value *_value);
};

/**
 * API call to simulate routine.
 * Provides pushed updates with queued targets, as they are recevied from
 * selector.
 *
 * This call depends on specific order of selector variables.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class AsyncSimulateAPI:public AsyncValueAPI
{
	public:	
		AsyncSimulateAPI (JSONRequest *_req, XmlRpc::XmlRpcServerConnection *_source, XmlRpc::HttpParams *params);
	private:
};

class AsyncDataAPI:public AsyncAPI
{
	public:
		AsyncDataAPI (JSONRequest *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, rts2core::DataAbstractRead *_data, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType);
		virtual void fullDataReceived (rts2core::Connection *_conn, rts2core::DataChannels *data);

		virtual void nullSource () { data = NULL; AsyncAPI::nullSource (); }

		virtual int idle ();

	protected:
		rts2core::DataAbstractRead *data;
		int channel;
		size_t bytesSoFar;

		long smin;
		long smax;

		rts2image::scaling_type scaling;
		int newType;
		int oldType;

		void sendData ();

	private:
		bool headerSend;

		void doSendData (void *buf, size_t bufs)
		{
			ssize_t ret = send (source->getfd (), buf, bufs, 0);
			if (ret < 0)
			{
				if (errno != EAGAIN && errno != EINTR)
				{
					logStream (MESSAGE_ERROR) << "cannot send data to client " << strerror (errno) << sendLog;
					asyncFinished ();
					return;
				}
			}
			else
			{
				bytesSoFar += ret;
			}
		}
};

class AsyncCurrentAPI:public AsyncDataAPI
{
	public:
		AsyncCurrentAPI (JSONRequest *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, rts2core::DataAbstractRead *_data, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType);
		virtual ~AsyncCurrentAPI ();

		virtual void dataReceived (rts2core::Connection *_conn, rts2core::DataAbstractRead *_data);
		virtual void exposureFailed (rts2core::Connection *_conn, int status);
};

class AsyncExposeAPI:public AsyncDataAPI
{
	public:
		AsyncExposeAPI (JSONRequest *_req, rts2core::Connection *conn, XmlRpc::XmlRpcServerConnection *_source, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType);
		virtual ~AsyncExposeAPI ();

		virtual void postEvent (rts2core::Event *event);

		virtual void dataReceived (rts2core::Connection *_conn, rts2core::DataAbstractRead *_data);

		virtual void fullDataReceived (rts2core::Connection *_conn, rts2core::DataChannels *data);
		virtual void exposureFailed (rts2core::Connection *_conn, int status);

		virtual int idle ();

	private:
		enum {waitForExpReturn, waitForImage, receivingImage} callState;
};

class AsyncAPIMSet:public AsyncAPI
{
	public:
		AsyncAPIMSet (JSONRequest *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, bool _ext): AsyncAPI (_req, _conn, _source, _ext)
		{
			pendingCalls = 0;
			succ = 0;
			failed = 0;
		}

		virtual void postEvent (rts2core::Event *event);

		void incCalls () { pendingCalls++; }
		void incSucc (int v) { succ += v; }
	private:
		int pendingCalls;
		int succ;
		int failed;
};

}

#endif // !__RTS2_ASYNCAPI__
