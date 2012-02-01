/* 
 * Set of accounts.
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

#include "account.h"

#include <string>
#include <map>

namespace rts2db 
{

/**
 * Represents a set of time accounts. Used to enable quick access to an account
 * with a given account id.
 *
 * As this class represents single table in RTS2 database, it uses singleton
 * instance with a protected constructor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AccountSet: public std::map < int, Account * >
{
	private:
		// Singleton entry.
		static AccountSet *pInstance;
		float shareSum;

	protected:
		/**
		 * Create new account set.
		 */
		AccountSet ();

	public:
		~AccountSet ();

		/**
		 * Load account set from database.
		 *
		 * @throw SqlError on error accessing database.
		 */
		void load ();

		/**
		 * Return sum of accounts shares.
		 */
		double getShareSum ()
		{
			return shareSum;
		}

		/**
		 * Singleton access routine.
		 */
		static AccountSet *instance ();
};


}
