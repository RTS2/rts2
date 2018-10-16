/* 
 * Routines for information values.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_INFOVAL__
#define __RTS2_INFOVAL__

#include <ostream>
#include <iomanip>

#include "libnova_cpp.h"
#include "timestamp.h"

/**
 * Prints pretty formate RTS2 values on output.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
template < class val > class InfoVal
{
	public:
		InfoVal (const char *in_desc, val in_value)
		{
			desc = in_desc;
			value = in_value;
		}

		const char* getDesc ()
		{
			return desc;
		}

		val& getValue ()
		{
			return value;
		}

	private:
		const char *desc;
		val value;

		template < class v > friend std::ostream & operator<< (std::ostream & _os, InfoVal < v > _val);
};

/**
 * Class for printing infoval.
 * This class prints infoval values. It provides virtual operators, which can be overloaded.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2InfoValStream
{
	public:
		Rts2InfoValStream ()
		{
		}

		virtual ~Rts2InfoValStream (void)
		{
		}

		virtual void printInfoVal (const char *desc, const char* val) = 0;

		virtual void printInfoVal (const char *desc, const std::string& val) = 0;

		virtual void printInfoVal (const char *desc, int val) = 0;

		virtual void printInfoVal (const char *desc, unsigned int val) = 0;

		virtual void printInfoVal (const char *desc, double val) = 0;

		virtual void printInfoVal (const char *desc, LibnovaDeg &val) = 0;

		virtual void printInfoVal (const char *desc, LibnovaRa &val) = 0;

		virtual void printInfoVal (const char *desc, LibnovaDegArcMin &val) = 0;

		virtual void printInfoVal (const char *desc, Timestamp &val) = 0;

		virtual void printInfoVal (const char *desc, TimeDiff &val) = 0;

		template < class v > Rts2InfoValStream& operator << (InfoVal <v> _val);

		virtual Rts2InfoValStream& operator << (__attribute__ ((unused)) const char* val)
		{
			return *this;
		}

		// for std::endl
		virtual Rts2InfoValStream& operator<< (__attribute__ ((unused)) std::ostream& (*f)(std::ostream&))
		{
			return *this;
		}

		/**
		 * Returns stream to print to..
		 */
		virtual std::ostream* getStream ()
		{
			return NULL;
		}
};

/**
 * Puts infoval to ostrem.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2InfoValOStream: public Rts2InfoValStream
{
	private:
		std::ostream* _os;
	public:
		Rts2InfoValOStream (std::ostream *in_os): Rts2InfoValStream ()
		{
			_os = in_os;
		}

		virtual void printInfoVal (const char *desc, const char* val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, const std::string &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, int val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, unsigned int val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, double val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, LibnovaDeg &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, LibnovaRa &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, LibnovaDegArcMin &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, Timestamp &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual void printInfoVal (const char *desc, TimeDiff &val)
		{
			*_os << std::left << std::setw (20) << desc
				<< std::right << val << std::endl;
		}

		virtual Rts2InfoValStream& operator << (const char* val)
		{
			*_os << val;
			return *this;
		}

		virtual Rts2InfoValStream& operator<< (std::ostream& (*f)(std::ostream&))
		{
			*_os << f;
			return *this;
		}

		virtual std::ostream* getStream ()
		{
			return _os;
		}
};

template < class v > Rts2InfoValStream& Rts2InfoValStream::operator << (InfoVal <v> _val)
{
	printInfoVal (_val.getDesc (), _val.getValue ());
	return *this;
}


template < class v > std::ostream & operator << (std::ostream & _os, InfoVal < v > _val)
{
	_os << std::left << std::setw (20) << _val.desc
		<< std::right << _val.value << std::endl;
	return _os;
}
#endif
