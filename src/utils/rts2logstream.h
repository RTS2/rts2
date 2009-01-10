/* 
 * Log steam, used for logging output.
 * Copyright (C) 2006-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_LOGSTREAM__
#define __RTS2_LOGSTREAM__

#include <sstream>

#include <message.h>

class Rts2App;

/**
 * Class used for streaming log messages. This class provides operators which
 * sends through it various values. Once the message is completed by sending
 * sendLog manipulator to this class, it is passed to the system for
 * processing.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2LogStream
{
	private:
		Rts2App * masterApp;
		messageType_t messageType;
		std::ostringstream ls;
	public:

		Rts2LogStream (Rts2App * in_master, messageType_t in_type)
		{
			masterApp = in_master;
			messageType = in_type;
			ls.setf (std::ios_base::fixed, std::ios_base::floatfield);
			ls.precision (6);
		}

		Rts2LogStream (const Rts2LogStream &_logStream)
		{
			masterApp = _logStream.masterApp;
			messageType = _logStream.messageType;
			ls.setf (std::ios_base::fixed, std::ios_base::floatfield);
			ls.precision (6);
		}


		Rts2LogStream (Rts2LogStream & _logStream)
		{
			masterApp = _logStream.masterApp;
			messageType = _logStream.messageType;
			ls.setf (std::ios_base::fixed, std::ios_base::floatfield);
			ls.precision (6);
		}

		Rts2LogStream & operator << (Rts2LogStream & (*func) (Rts2LogStream &))
		{
			return func (*this);
		}

		template < typename _charT > Rts2LogStream & operator << (_charT value)
		{
			ls << value;
			return *this;
		}

		/**
		 * Set fill value for log stream. Call fill method for ostream.
		 *
		 * @param _f  New fill value.
		 *
		 * @return Old fill character.
		 */
		char fill (char _f)
		{
			return ls.fill (_f);
		}

		/**
		 * Log character array as array of character and hex values
		 * when it cannot be represented as character.  Hex values are
		 * prefixed with 0x and separated by blanks.
		 *
		 * @param arr  Array which will be logged.  @param len  Lenght
		 * of logged array.
		 */
		void logArr (char *arr, int len);

		/**
		 * Log character array as array of hex values (prefixed with 0x and separated by blanks)
		 *
		 * @param arr  Array which will be logged.
		 * @param len  Lenght of logged array.
		 */
		void logArrAsHex (char *arr, int len);

		inline void sendLog ();
};

/**
 * Send log. That is used as manipulator for stream to send it through the system.
 */
Rts2LogStream & sendLog (Rts2LogStream & _ls);
#endif							 /* ! __RTS2_LOGSTREAM__ */
