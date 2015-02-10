/* 
 * Value changes triggering infrastructure. 
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

#ifndef __RTS2__VALUEEVENTS__
#define __RTS2__VALUEEVENTS__

#include "value.h"
#include "expression.h"

#include "emailaction.h"

#include <map>
#include <list>
#include <string>

using namespace rts2expression;

namespace rts2xmlrpc
{

/**
 * Class triggered on value change.
 */
class ValueChange:public rts2core::Object
{
	public:
		ValueChange (HttpD *_master, std::string _deviceName, std::string _valueName, float _cadency, Expression *test);

		/**
		 * Catch EVENT_XMLRPC_VALUE_TIMER events.
		 *
		 * @see rts2core::Object::postEvent(rts2core::Event*)
		 */
		virtual void postEvent (rts2core::Event * event);

		bool isForValue (std::string _deviceName, std::string _valueName, double infoTime)
		{
			if (deviceName == _deviceName.c_str () && valueName == _valueName.c_str () && (cadency < 0 || lastTime + cadency < infoTime))
			{
				if (test && test->evaluate () == 0)
						return false;
				return true;
			}
			return false;
		}

		/**
		 * Triggered when value is changed. Throws Errors on error.
		 */
		virtual void run (rts2core::Value *val, double validTime) = 0;

		/**
		 * Called at the end of run method, when command change was run succesfully.
		 */
		void runSuccessfully (double validTime)
		{
			lastTime = validTime;
		}

	protected:
		HttpD *master;

		ci_string deviceName;
		ci_string valueName;

	private:
		double lastTime;
		float cadency;
		Expression *test;
};

/**
 * Record value change, either to database (rts2-xmlrpcd is compiled with database support) or
 * to standard output (if rts2-xmlrpcd is compiled without database support).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueChangeRecord: public ValueChange
{
	public:
		ValueChangeRecord (HttpD *_master, std::string _deviceName, std::string _valueName, float _cadency, Expression *_test):ValueChange (_master, _deviceName, _valueName, _cadency, _test) {}

		virtual void run (rts2core::Value *val, double validTime);
#ifdef RTS2_HAVE_PGSQL
	private:
		std::map <const char *, int> dbValueIds;
		int getRecvalId (const char *suffix, int recval_type);
		void recordValueInteger (int recval_id, int val, double validTime);
		void recordValueDouble (int recval_id, double val, double validTime);
		void recordValueBoolean (int recval_id, bool val, double validTime);
#endif /* RTS2_HAVE_PGSQL */
};


/**
 * Run command on value change.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueChangeCommand: public ValueChange
{
	public:
		ValueChangeCommand (HttpD *_master, std::string _deviceName, std::string _valueName, float _cadency, Expression *_test, std::string _commandName):ValueChange (_master, _deviceName, _valueName, _cadency, _test)
		{
			commandName = _commandName;
		}

		virtual void run (rts2core::Value *val, double validTime);
	private:
		std::string commandName;
};

/**
 * Send email on value change.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueChangeEmail: public ValueChange, public EmailAction
{
	public:
		ValueChangeEmail (HttpD *_master, std::string _deviceName, std::string _valueName, float _cadency, Expression *_test):ValueChange (_master, _deviceName, _valueName, _cadency, _test), EmailAction () {}

		virtual void run (rts2core::Value *val, double validTime);
};

/**
 * Holds list of ValueChangeCommands
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueCommands:public std::list <ValueChange *>
{
	public:
		ValueCommands () {}

		~ValueCommands ()
		{
			for (ValueCommands::iterator iter = begin (); iter != end (); iter++)
				delete (*iter);
		}
};

}

#endif /* !__RTS2__VALUEEVENTS__ */
