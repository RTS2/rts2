/* 
 * Various API routines, usually XML-RPC API.
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

#ifndef __RTS2_XMLAPI__
#define __RTS2_XMLAPI__

#include "xmlrpc++/XmlRpc.h"
#include "rts2-config.h"
#include "r2x.h"
#include "block.h"

using namespace XmlRpc;

/**
 * Transform connection values to XMLRPC.
 */
void connectionValuesToXmlRpc (rts2core::Connection *conn, XmlRpcValue& result, bool pretty);

/**
 * Represents session methods. Those must be executed either with user name and
 * password, or with "session_id" passed as username and session ID passed in password.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class SessionMethod: public XmlRpcServerMethod
{
	public:
		SessionMethod (const char *method, XmlRpcServer* s);

		void execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result);

		virtual void sessionExecute (XmlRpcValue& params, XmlRpcValue& result) = 0;
	private:
		bool executePermission;
};

class Login: public XmlRpcServerMethod
{
	public:
		Login (XmlRpcServer* s): XmlRpcServerMethod (R2X_LOGIN, s) { executePermission = false; }

		virtual void execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Return session ID for user if logged properly");
		}
	private:
		bool executePermission;
};

/**
 * Returns number of devices connected to the system.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class DeviceCount: public SessionMethod
{
	public:
		DeviceCount (XmlRpcServer* s) : SessionMethod ("DeviceCount", s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns number of devices connected to XMLRPC");
		}
};

/**
 * List name of devices connected to the server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListDevices: public SessionMethod
{
	public:
		ListDevices (XmlRpcServer* s) : SessionMethod (R2X_DEVICES_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns names of devices conencted to the system");
		}
};

class DeviceByType: public SessionMethod
{
	public:
		DeviceByType (XmlRpcServer* s) : SessionMethod (R2X_DEVICE_GET_BY_TYPE, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns names of device of the given type, connected to the system");
		}
		
};

/**
 * Get device type. Returns string describing device type.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class DeviceType: public SessionMethod
{
	public:
		DeviceType (XmlRpcServer* s): SessionMethod (R2X_DEVICE_TYPE, s)
		{
		}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue &result);
};

/**
 * Execute command on device.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class DeviceCommand: public SessionMethod
{
	public:
		DeviceCommand (XmlRpcServer* s): SessionMethod (R2X_DEVICE_COMMAND, s)
		{
		}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue &result);
};

class MasterState: public SessionMethod
{
	public:
		MasterState (XmlRpcServer* s): SessionMethod (R2X_MASTER_STATE, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns master state");
		}
};

class MasterStateIs: public SessionMethod
{
	public:
		MasterStateIs (XmlRpcServer* s): SessionMethod (R2X_MASTER_STATE_IS, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

/**
 * List device status.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class DeviceState: public SessionMethod
{
	public:
		DeviceState (XmlRpcServer* s) : SessionMethod (R2X_DEVICE_STATE, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns state of devices");
		}
};

/**
 * List name of all values accessible from server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListValues: public SessionMethod
{
	public:
		ListValues (XmlRpcServer* s): SessionMethod (R2X_VALUES_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns name of devices connected to the system");
		}

	protected:
		ListValues (const char *in_name, XmlRpcServer* s): SessionMethod (in_name, s) {}
};

/**
 * List name of all values accessible from server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListValuesDevice: public ListValues
{
	public:
		ListValuesDevice (XmlRpcServer* s): ListValues (R2X_DEVICES_VALUES_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system for given device(s)");
		}
};

class ListPrettyValuesDevice: public ListValues
{
	public:
		ListPrettyValuesDevice (XmlRpcServer* s): ListValues (R2X_DEVICES_VALUES_PRETTYLIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system for given device(s)");
		}
};

class GetValue: public SessionMethod
{
	public:
		GetValue (XmlRpcServer* s) : SessionMethod (R2X_VALUE_GET, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class GetPrettyValue: public SessionMethod
{
	public:
		GetPrettyValue (XmlRpcServer* s) : SessionMethod (R2X_VALUE_PRETTY, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class SessionMethodValue:public SessionMethod
{
	protected:
		SessionMethodValue (const char *method, XmlRpcServer *s):SessionMethod (method, s) {}

		void setXmlValutRts2 (rts2core::Connection *conn, std::string valueName, XmlRpcValue &x_val);
};

class SetValue: public SessionMethodValue
{
	public:
		SetValue (XmlRpcServer* s) : SessionMethodValue (R2X_VALUE_SET, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Set RTS2 value");
		}

};

class SetValueByType: public SessionMethodValue
{
	public:
		SetValueByType (XmlRpcServer* s) : SessionMethodValue (R2X_VALUE_BY_TYPE_SET, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Set RTS2 value for device by type");
		}

};

class IncValue: public SessionMethod
{
	public:
		IncValue (XmlRpcServer* s) : SessionMethod (R2X_VALUE_INC, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Increment RTS2 value");
		}

};

class GetMessages: public SessionMethod
{
	public:
		GetMessages (XmlRpcServer* s) : SessionMethod (R2X_MESSAGES_GET, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

#ifdef RTS2_HAVE_PGSQL
/*
 *
 */
class ListTargets: public SessionMethod
{
	public:
		ListTargets (XmlRpcServer* s) : SessionMethod (R2X_TARGETS_LIST, s) {}

		virtual void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns all targets");
		}

};

class ListTargetsByType: public SessionMethod
{
	public:
		ListTargetsByType (XmlRpcServer* s) : SessionMethod (R2X_TARGETS_TYPE_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Returns all targets");
		}

};

class TargetInfo: public SessionMethod
{
	public:
		TargetInfo (XmlRpcServer* s) : SessionMethod (R2X_TARGETS_INFO, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class TargetAltitude: public SessionMethod
{
	public:
		TargetAltitude (XmlRpcServer *s) : SessionMethod (R2X_TARGET_ALTITUDE, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("Return 2D array with informations about target height at diferent times.");
		}
};

class ListTargetObservations: public SessionMethod
{
	public:
		ListTargetObservations (XmlRpcServer* s) : SessionMethod (R2X_TARGET_OBSERVATIONS_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);

		std::string help ()
		{
			return std::string ("returns observations of given target");
		}
};

class ListMonthObservations: public SessionMethod
{
	public:
		ListMonthObservations (XmlRpcServer* s) : SessionMethod (R2X_OBSERVATIONS_MONTH, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class ListImages: public SessionMethod
{
	public:
		ListImages (XmlRpcServer* s) : SessionMethod (R2X_OBSERVATION_IMAGES_LIST, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class TicketInfo: public SessionMethod
{
	public:
		TicketInfo (XmlRpcServer *s): SessionMethod (R2X_TICKET_INFO, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class RecordsValues: public SessionMethod
{
	public:
		RecordsValues (XmlRpcServer* s): SessionMethod (R2X_RECORDS_VALUES, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class Records: public SessionMethod
{
	public:
		Records (XmlRpcServer* s): SessionMethod (R2X_RECORDS_GET, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class RecordsAverage: public SessionMethod
{
	public:
		RecordsAverage (XmlRpcServer* s): SessionMethod (R2X_RECORDS_AVERAGES, s) {}

		void sessionExecute (XmlRpcValue& params, XmlRpcValue& result);
};

class UserLogin: public XmlRpcServerMethod
{
	public:
		UserLogin (XmlRpcServer* s) : XmlRpcServerMethod (R2X_USER_LOGIN, s) { executePermission = false; }

		void execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result);

	private:
		bool executePermission;
};

#endif /* RTS2_HAVE_PGSQL */

#endif // __RTS2_XMLAPI__
