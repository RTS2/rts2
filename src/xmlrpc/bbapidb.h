/* 
 * BB API database support.
 * Copyright (C) 2013 Petr Kubanek <petr@kubanek.net>
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

namespace rts2xmlrpc
{

class BBSchedule
{
	public:
		BBSchedule (std::string _schedule_id, int _target_id, double _from = NAN, double _to = NAN)
		{
			schedule_id = _schedule_id;
			target_id = _target_id;
			from = _from;
			to = _to;
			quid = -1;
		}

	private:
		std::string schedule_id;
		int target_id;
		double from;
		double to;
		int quid;
};

class BBSchedules:public std::map <std::string, BBSchedule *>
{
	public:
		BBSchedules () {}

		~BBSchedules ()
		{
			for (BBSchedules::iterator iter = begin (); iter != end (); iter++)
				delete iter->second;				
		}
};

}
