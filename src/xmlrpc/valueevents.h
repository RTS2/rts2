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

#include "../utils/rts2value.h"

#include "emailaction.h"

#include <map>
#include <list>
#include <string>

namespace rts2xmlrpc
{

/**
 * Class triggered on value change.
 */
class ValueChange
{
	public:
		ValueChange (std::string _deviceName, std::string _valueName, float _cadency)
		{
			deviceName = _deviceName;
			valueName = _valueName;
			
			lastTime = 0;
			cadency = _cadency;
		}

		bool isForValue (std::string _deviceName, std::string _valueName, double infoTime)
		{
			return deviceName == _deviceName && valueName == _valueName && (cadency < 0 || lastTime + cadency < infoTime);
		}

		/**
		 * Triggered when value is changed. Throws Errors on error.
		 */
		virtual void run (XmlRpcd *_master, Rts2Value *val, double validTime) = 0;

		/**
		 * Called at the end of run method, when command change was run succesfully.
		 */
		void runSuccessfully (double validTime)
		{
			lastTime = validTime;
		}

	protected:
		std::string deviceName;
		std::string valueName;

	private:
		double lastTime;
		float cadency;
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
		ValueChangeRecord (std::string _deviceName, std::string _valueName, float _cadency):ValueChange (_deviceName, _valueName, _cadency)
		{
		}

		virtual void run (XmlRpcd *_master, Rts2Value *val, double validTime);
#ifdef HAVE_PGSQL
	private:
		std::map <const char *, int> dbValueIds;
		int getRecvalId (const char *suffix = NULL);
		void recordValueDouble (int recval_id, double val, double validTime);
		void recordValueBoolean (int recval_id, bool val, double validTime);
#endif /* HAVE_PGSQL */
};


/**
 * Run command on value change.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueChangeCommand: public ValueChange
{
	public:
		ValueChangeCommand (std::string _deviceName, std::string _valueName, float _cadency, std::string _commandName):ValueChange (_deviceName, _valueName, _cadency)
		{
			commandName = _commandName;
		}

		virtual void run (XmlRpcd *_master, Rts2Value *val, double validTime);
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
		ValueChangeEmail (std::string _deviceName, std::string _valueName, float _cadency):ValueChange (_deviceName, _valueName, _cadency), EmailAction ()
		{
		}

		virtual void run (XmlRpcd *_master, Rts2Value *val, double validTime);
};

/**
 * Holds list of ValueChangeCommands
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueCommands:public std::list <ValueChange *>
{
	public:
		ValueCommands ()
		{
		}

		~ValueCommands ()
		{
			for (ValueCommands::iterator iter = begin (); iter != end (); iter++)
				delete (*iter);
		}
};

}

#endif /* !__RTS2__VALUEEVENTS__ */
