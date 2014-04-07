/* 
 * Class for reporting sql errors.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SQLERR__
#define __RTS2_SQLERR__

#include <string>
#include "error.h"

namespace rts2db {

/**
 * Prints informations about SQL error message to ostream.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SqlError: public rts2core::Error
{
	public:
		SqlError ();
		SqlError (const char *sqlmsg);

		/**
		 * Returns SQL error code.
		 */
		int getSqlCode () { return sqlcode; }

	private:
		int sqlcode;
};

}

#endif // ! __RTS2_SQLERR__
