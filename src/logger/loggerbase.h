/* 
 * Logger base.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "devclient.h"
#include "displayvalue.h"
#include "command.h"
#include "expander.h"
#include "utilsfunc.h"

#define EVENT_SET_LOGFILE RTS2_LOCAL_EVENT+800

namespace rts2logd
{

/**
 * This class logs given values to given file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Logger
 */
class DevClientLogger:public rts2core::DevClient
{
	public:
		/**
		 * Construct client for logging device.
		 *
		 * @param in_conn                  Connection.
		 * @param in_numberSec             Number of seconds when the info command will be send.
		 * @param in_fileCreationInterval  Interval between file creation.
		 * @param in_logNames              String with space separated names of values which will be logged.
		 */
		DevClientLogger (rts2core::Rts2Connection * in_conn, double in_numberSec, time_t in_fileCreationInterval, std::list < std::string > &in_logNames);

		virtual ~ DevClientLogger (void);
		virtual void infoOK ();
		virtual void infoFailed ();

		virtual void idle ();

		virtual void postEvent (rts2core::Event * event);
	protected:
		/**
		 * Change expansion pattern to new filename.
		 *
		 * @param pattern Expanding pattern.
		 */
		void setOutputFile (const char *pattern);
	private:
		std::list < rts2core::Value * >logValues;
		std::list < std::string > logNames;

		std::ostream * outputStream;

		rts2core::Expander * exp;
		std::string expandPattern;
		std::string expandedFilename;

		/**
		 * Next time info command will be called.
		 */
		struct timeval nextInfoCall;

		/**
		 * Info command will be send every numberSec seconds.
		 */
		struct timeval numberSec;

		/**
		 * Interval between two sucessive tests of file expansion.
		 */
		time_t fileCreationInterval;

		/**
		 * Next file creation check.
		 */
		time_t nextFileCreationCheck;
	
		/**
		 * Fill to log file values which are in logValues array.
		 */
		void fillLogValues ();

		/**
		 * Change output stream according to new expansion.
		 */
		void changeOutputStream ();
};

/**
 * Holds informations about which device should log which values at which time interval.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class LogValName
{
	public:
		std::string devName;
		double timeout;
		std::list < std::string > valueList;

		LogValName (std::string in_devName, double in_timeout, std::list < std::string > in_valueList)
		{
			devName = in_devName;
			timeout = in_timeout;
			valueList = in_valueList;
		}
};

/**
 * Bse class for logging. Provides functions for both rts2core::App and rts2core::Device, which
 * shall be called from the respective program bodies.
 *
 * @ingroup RTS2Logger
 * @author Petr Kubanek <petr@kubanek.net>
 */
class LoggerBase
{
	public:
		LoggerBase ();
		rts2core::DevClient *createOtherType (rts2core::Rts2Connection * conn, int other_device_type);
	protected:
		int readDevices (std::istream & is);

		LogValName *getLogVal (const char *name);
		int willConnect (rts2core::NetworkAddress * in_addr);
	private:
		std::list < LogValName > devicesNames;
};

}
