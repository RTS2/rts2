/* 
 * Filter look-up table and creator.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DBFILTERS__
#define __RTS2_DBFILTERS__

#include <map>
#include <string>

namespace rts2image
{
/**
 * Class managing filter cache. It can also insert new filters into database.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DBFilters:public std::map <int, std::string>
{
	public:
		DBFilters ();

		static DBFilters * instance ();

		/**
		 * Load filters from database.
		 */
		void load ();

		/**
		 * Get index of filter with given name. Create new entry in
		 * filter table if the filter is not know.
		 */
		int getIndex (const char *filer);

	private:
		static DBFilters *pInstance;
};

}

#endif							 /* ! __RTS2_DBFILTERS__ */
