/* 
 * EPICS interface.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNEPICS__
#define __RTS2_CONNEPICS__

#include "../utils/rts2connnosend.h"

#include <cadef.h>
#include <map>

namespace rts2core
{

/**
 * EPICS connection error.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpicsError
{
	private:
		const char *message;
		int result;
	protected:
		const char *getMessage ()
		{
			return message;
		}

		const char *getError ()
		{
			return ca_message (result);
		}
	public:
		ConnEpicsError (const char *_message, int _result)
		{
			message = _message;
			result = _result;
		}

		friend std::ostream & operator << (std::ostream &_os, ConnEpicsError &_epics)
		{
			_os << _epics.getMessage () << _epics.getError ();
			return _os;
		}
};


/**
 * Error class for channles.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpicsErrorChannel:public ConnEpicsError
{
	private:
		const char *pvname;
	public:
		ConnEpicsErrorChannel (const char *_message, const char *_pvname, int _result)
		:ConnEpicsError (_message, _result)
		{
			pvname = _pvname;
		}

		friend std::ostream & operator << (std::ostream &_os, ConnEpicsErrorChannel &_epics)
		{
			_os << _epics.getMessage () << ", channel " << _epics.pvname << " error " << _epics.getError ();
			return _os;
		}
};


/**
 * Interface between class and channel value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class EpicsVal
{
	public:
		Rts2Value *value;
		chid vchid;
		void *storage;

		EpicsVal (Rts2Value *_value, chid _vchid)
		{
			value = _value;
			vchid = _vchid;
			storage = NULL;
		}

		~EpicsVal ()
		{
			if (storage != NULL)
				free (storage);
		}
};


/**
 * Connection for EPICS clients.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpics: public Rts2ConnNoSend
{
	private:
		std::list <EpicsVal> channels;

		EpicsVal * findValue (Rts2Value *value);
	public:
		ConnEpics (Rts2Block *_master);

		virtual ~ConnEpics ();

		virtual int init ();

		/**
		 * Associate Rts2Value with channel.
		 */
		void addRts2Value (Rts2Value *value, const char *pvname);

		void addRts2Value (Rts2Value *value, const char *prefix, const char *pvname)
		{
			addRts2Value (value, (std::string (prefix) + std::string (pvname)).c_str ());
		}

		chid createChannel (const char *pvname);

		chid createChannel (const char *prefix, const char *pvname)
		{
			return createChannel ((std::string (prefix) + std::string (pvname)).c_str ());
		}

		/**
		 * Deletes channel.
		 */
		void deleteChannel (chid _ch);

		void get (chid _ch, double *val);

		/**
		 * This function request value update. You are responsible to
		 * call ConnEpics::callPendIO() to actually perform the
		 * request. Value will not be update before you call
		 * ConnEpics::callPendIO().
		 *
		 * @param value Value which is requested.
		 */
		void queueGetValue (Rts2Value *value);

		/**
		 * Queue value for set operation.
		 */
		void queueSetValue (Rts2Value *value);

		/**
		 * Set integer value.
		 */
		void queueSet (chid _vchid, int value);
		void queueSet (chid _vchid, double value);

		void queueSetEnum (chid _vchid, int value);

		void callPendIO ();
};

};

#endif /* !__RTS2_CONNEPICS__ */
