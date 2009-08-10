/* 
 * Top error class. Its descendats are thrown when some HW error occurs.
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
class Error
{
	public:
		Error ()
		{
		}

		Error (const char *_msg)
		{
			msg = std::string (_msg);
		}

		Error (std::string _msg)
		{
			msg = _msg;
		}

		/**
		 * Returns message associated with the error.
		 */
		std::string getMsg ()
		{
			return msg;
		}

		friend std::ostream & operator << (std::ostream &_os, Error _err)
		{
			_os << "error: " << _err.getMsg ();
			return _os;
		}

	protected:
		void setMsg (std::string _msg)
		{
			msg = _msg;
		}

	private:
		std::string msg;
};

};

#endif /* __RTS2_ERROR__ */
