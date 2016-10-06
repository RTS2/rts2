/* 
 * Top error class. Its descendats are thrown when some HW error occurs.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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
#ifndef __RTS2_ERROR__
#define __RTS2_ERROR__

#include <exception>
#include <string>
#include <ostream>

namespace rts2core
{

/**
 * Superclass for any errors.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Error:public std::exception
{
	public:
		explicit Error (): std::exception () {}

		explicit Error (const char *_msg): std::exception () { msg = std::string (_msg); }

		explicit Error (const char *_msg, const char *arg1): std::exception () { msg = std::string (_msg) + " " + arg1; }

		explicit Error (std::string _msg): std::exception () { msg = _msg; }

		virtual ~Error() throw() {};

		/**
		 * Returns message associated with the error.
		 */
		virtual const char* what () const throw () { return msg.c_str (); }

		friend std::ostream & operator << (std::ostream &_os, Error _err)
		{
			_os << "error: " << _err.what ();
			return _os;
		}

	protected:
		void setMsg (std::string _msg) { msg = _msg; }

	private:
		std::string msg;
};

}

#endif /* __RTS2_ERROR__ */
