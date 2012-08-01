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

#include "asyncapi.h"
#include "api.h"
#include "jsonvalue.h"

using namespace rts2xmlrpc;

AsyncAPI::AsyncAPI (API *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, bool _ext):Object ()
{
	// that's legal - requests are statically allocated and will cease exists with the end of application
	req = _req;
	conn = _conn;
	source = _source;
	ext = _ext;
}

AsyncAPI::~AsyncAPI ()
{
	if (source)
		source->asyncFinished ();
}

void AsyncAPI::postEvent (rts2core::Event *event)
{
	std::ostringstream os;
	if (source)
	{
		switch (event->getType ())
		{
			case EVENT_COMMAND_OK:
				os << "{";
				req->sendConnectionValues (os, conn, NULL, -1, ext);
				os << ",\"ret\":0}";
				req->sendAsyncJSON (os, source);
				asyncFinished ();
				break;
			case EVENT_COMMAND_FAILED:
				os << "{";
				req->sendConnectionValues (os, conn, NULL, -1, ext);
				os << ",\"ret\":-1}";
				req->sendAsyncJSON (os, source);
				asyncFinished ();
				break;
		}
	}
	Object::postEvent (event);
}

AsyncValueAPI::AsyncValueAPI (API *_req, XmlRpc::XmlRpcServerConnection *_source, XmlRpc::HttpParams *params): AsyncAPI (_req, NULL, _source, false) 
{
	// chunked response
	req->sendAsyncDataHeader (0, _source, "application/json");

	for (XmlRpc::HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
		values.push_back (std::pair <std::string, std::string> (iter->getName (), iter->getValue ()));
}

void AsyncValueAPI::valueChanged (rts2core::Connection *_conn, rts2core::Value *_value)
{
	for (std::vector <std::pair <std::string, std::string> >::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		if (iter->first == _conn->getName () && _value->isValue (iter->second.c_str ()) && source != NULL)
		{
			sendValue (iter->first, _value);
			return;
		}
	}
}

void AsyncValueAPI::sendAllValues (rts2core::Device *device)
{
	rts2core::Value *val;
	for (std::vector <std::pair <std::string, std::string> >::iterator iter = values.begin (); iter != values.end (); iter++)
	{
		if (iter->first == device->getDeviceName ())
		{
			val = device->getOwnValue (iter->second.c_str ());
		}
		else
		{
			rts2core::Connection *con = device->getOpenConnection (iter->first.c_str ());
			if (con == NULL)
				throw XmlRpc::JSONException ("cannot find open connection with name " + iter->first);
			val = con->getValue (iter->second.c_str ());
		}
		if (val == NULL)
			throw XmlRpc::JSONException ("cannot find value " + iter->first + "." + iter->second);
		sendValue (iter->first, val);
	}
}

void AsyncValueAPI::sendValue (const std::string &device, rts2core::Value *_value)
{
	std::ostringstream os;
	os << "{\"d\":\"" << device << "\",\"v\":{";
	jsonValue (_value, false, os);
	os << "}}";
	source->sendChunked (os.str ());
}

AsyncDataAPI::AsyncDataAPI (API *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, rts2core::DataAbstractRead *_data, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType):AsyncAPI (_req, _conn, _source, false)
{
	data = _data;
	channel = _chan;
	bytesSoFar = 0;
	smin = _smin;
	smax = _smax;

	scaling = _scaling;
	newType = _newType;
	oldType = 0;

	headerSend = false;
}

void AsyncDataAPI::fullDataReceived (rts2core::Connection *_conn, rts2core::DataChannels *_data)
{
	AsyncAPI::fullDataReceived (_conn, _data);

	if (isForConnection (_conn))
	{
		rts2core::DataChannels::iterator iter = std::find (_data->begin (), _data->end (), data);
		if (iter != _data->end ())
		{
			if (source)
			{
				size_t bosend = bytesSoFar;
				if (newType != 0 && newType != oldType)
				{
					bosend = sizeof (struct imghdr) + (oldType / 8) * (bosend - sizeof (struct imghdr)) / (newType / 8);
				}
				if (data->getRestSize () > 0)
				{
					// incomplete image was received, close outbond connection..
					source->close ();
					nullSource ();
				}
				else if (bosend < (size_t) (data->getDataTop () - data->getDataBuff ()))
				{
					// full image was received, let's make sure it will be send
					if (newType != 0 && newType != oldType)
					{
						size_t ds = data->getDataTop () - data->getDataBuff () - bosend;
						char *newData = new char[ds];
						memcpy (newData, data->getDataBuff () + bosend, ds);
						// scale data for async
						if (bytesSoFar < sizeof (struct imghdr))
						{
							struct imghdr imgh;
							memcpy (&imgh, newData, sizeof (struct imghdr) - bytesSoFar);
							imgh.data_type = htons (newType);

							oldType = ntohs (((struct imghdr *) data->getDataBuff ())->data_type);
							ds = (ds - sizeof (struct imghdr) + bytesSoFar) / (oldType / 8);

							getScaledData (oldType, newData + sizeof (struct imghdr) - bytesSoFar, ds, smin, smax, scaling, newType);
							ds = sizeof (struct imghdr) + (newType / 8) * ds;
							if (bytesSoFar == 0 && headerSend == false)
							{
								req->sendAsyncDataHeader (ds, source);
								headerSend = true;
							}
						}
						else
						{
							ds /= (oldType / 8);
							// we send out pixels, so this scaling is legal
							getScaledData (oldType, newData, ds, smin, smax, scaling, newType);
							ds *= (newType / 8);
						}
						source->setResponse (newData, ds);
						delete[] newData;
					}
					else
					{
						source->setResponse (data->getDataBuff () + bytesSoFar, data->getDataTop () - data->getDataBuff () - bytesSoFar);
					}
					nullSource ();
				}
				else
				{
					asyncFinished ();
				}
			}
			return;
		}
	}
}

int AsyncDataAPI::idle ()
{
	// see new data on shared connection
	if (data && (data->getDataTop () - data->getDataBuff ()) > bytesSoFar)
	{
		dataReceived (conn, data);
	}
	return AsyncAPI::idle ();
}

void AsyncDataAPI::sendData ()
{
	if (bytesSoFar == 0)
	{
		size_t ds = data->getDataTop () - data->getDataBuff () + data->getRestSize ();
		if (headerSend == false && ds >= sizeof (struct imghdr))
		{
			if (newType != 0)
			{
				if (oldType == 0)
					oldType = ntohs (((struct imghdr *) data->getDataBuff ())->data_type);

				req->sendAsyncDataHeader (sizeof (struct imghdr) + (newType / 8) * (ds - sizeof (struct imghdr)) / (oldType / 8), source);
			}
			else
			{
				req->sendAsyncDataHeader (ds, source);
			}
			headerSend = true;
		}
	}
	// if scaling is needed, wait for full image header
	if (newType != 0 && newType != oldType)
	{
		if (bytesSoFar < sizeof (struct imghdr))
		{
			if (data->getDataTop () - data->getDataBuff () >= (ssize_t) (sizeof (struct imghdr)))
			{
				struct imghdr imgh;
				memcpy (&imgh, data->getDataBuff (), sizeof (struct imghdr));
				imgh.data_type = htons (newType);
				doSendData (((char *) (&imgh)) + bytesSoFar, sizeof (struct imghdr) - bytesSoFar);
			}
		}
		// bytesSoFar > sizeof (struct imghdr)
		else
		{
			// send bytes in old data lenght
			size_t bosend = sizeof (struct imghdr) + (oldType / 8) * (bytesSoFar - sizeof (struct imghdr)) / (newType / 8);
			size_t ds = data->getDataTop () - data->getDataBuff () - bosend;
			char *newData = new char[ds];
			memcpy (newData, data->getDataBuff () + bosend, ds);
			// scale data
			ds /= (oldType / 8);
			getScaledData (oldType, newData, ds, smin, smax, scaling, newType);
			doSendData (newData, (newType / 8) * ds);
			delete[] newData;
		}
		return;
	}
	doSendData (data->getDataBuff () + bytesSoFar, data->getDataTop () - data->getDataBuff () - bytesSoFar);
}

AsyncCurrentAPI::AsyncCurrentAPI (API *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, rts2core::DataAbstractRead *_data, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType):AsyncDataAPI (_req, _conn, _source, _data, _chan, _smin, _smax, _scaling, _newType)
{
	// try to send data
	sendData ();
}

AsyncCurrentAPI::~AsyncCurrentAPI ()
{
}

void AsyncCurrentAPI::dataReceived (rts2core::Connection *_conn, rts2core::DataAbstractRead *_data)
{
	if (_data == data)
		sendData ();
}

void AsyncCurrentAPI::exposureFailed (rts2core::Connection *_conn, int status)
{
	if (isForConnection (_conn))
	{
		if (source)
		{
			source->close ();
		}
		asyncFinished ();
	}
}

AsyncExposeAPI::AsyncExposeAPI (API *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, int _chan, long _smin, long _smax, rts2image::scaling_type _scaling, int _newType):AsyncDataAPI (_req, _conn, _source, NULL, _chan, _smin, _smax, _scaling, _newType)
{
	callState = waitForExpReturn;
}

AsyncExposeAPI::~AsyncExposeAPI ()
{
}
 
void AsyncExposeAPI::postEvent (rts2core::Event *event)
{
	if (source)
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
					asyncFinished ();
				}
				break;
		}
	}
	Object::postEvent (event);
}

void AsyncExposeAPI::dataReceived (rts2core::Connection *_conn, rts2core::DataAbstractRead *_data)
{
	if (_data == data)
		sendData ();
	else if (data == NULL && isForConnection (_conn) && callState == waitForImage)
	{
		callState = receivingImage;
		data = _data;
		if (data == NULL)
		{
			asyncFinished ();
			return;
		}
		sendData ();
	}
}

void AsyncExposeAPI::fullDataReceived (rts2core::Connection *_conn, rts2core::DataChannels *_data)
{
	// in case idle loop was not called
	if (isForConnection (_conn) && callState == waitForImage && data == NULL)
	{
		callState = receivingImage;
		data = _conn->lastDataChannel (channel);
	}

	AsyncDataAPI::fullDataReceived (_conn, _data);
}

void AsyncExposeAPI::exposureFailed (rts2core::Connection *_conn, int status)
{
	if (isForConnection (_conn))
	{
		if (source)
		{
			switch (callState)
			{
				case waitForExpReturn:
				case waitForImage:
					{
						std::ostringstream os;
						os << "{\"failed\"}";
						req->sendAsyncJSON (os, source);
					}
					break;
				case receivingImage:
					if (source)
						source->close ();
					break;
			}
			asyncFinished ();
		}
	}
}

int AsyncExposeAPI::idle ()
{
	if (data == NULL && callState == waitForImage && conn->lastDataChannel (channel) && conn->lastDataChannel (channel)->getRestSize () > 0)
	{
		callState = receivingImage;
		data = conn->lastDataChannel (channel);
	}

	return AsyncDataAPI::idle ();
}

void AsyncAPIMSet::postEvent (rts2core::Event *event)
{
	std::ostringstream os;
	if (source)
	{
		switch (event->getType ())
		{
			case EVENT_COMMAND_OK:
				pendingCalls--;
				succ++;
				break;
			case EVENT_COMMAND_FAILED:
				pendingCalls--;
				failed++;
				break;
		}

		if (pendingCalls == 0 && (succ != 0 || failed != 0))
		{
			os << "{\"succ\":" << succ << ",\"failed\":" << failed << ",\"ret\":0}";
			req->sendAsyncJSON (os, source);
			asyncFinished ();
		}
	}
	Object::postEvent (event);
}
