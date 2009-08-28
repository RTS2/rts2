/* 
 * Account class.
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

#include <string>

namespace rts2db 
{

/**
 * Represent time accounts. Accounts are created for various users groups to
 * hold informations about their share of scheduling time.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Account
{
	private:
		// account Id
		int id;

		// account name
		std::string name;

		float share;

	public:
		/**
		 * Create an account with given account id.
		 *
		 * @param _id  Account id.
		 */
		Account (int _id);

		/**
		 * Create an account with given account id and account name.
		 *
		 * @param _id  Account id.
		 * @param _name Account name.
		 * @param _share Account target time share in percents.
		 */
		Account (int _id, std::string _name, float _share);

		/**
		 * Load account from database.
		 *
		 * @return -1 if account entry cannot be found, 0 on success.
		 */
		int load ();

		/**
		 * Return expected account share.
		 */
		float getShare ()
		{
			return share;
		}
};


}
