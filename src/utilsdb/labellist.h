/* 
 * List all labels know to the system
 * Copyright (C) 2011 Petr Kubanek, Insititute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_LABELLIST__
#define __RTS2_LABELLIST__

#include <string>
#include <vector>

namespace rts2db
{

class LabelEntry
{
	public:
		LabelEntry (int l, int t, const char *te)
		{
			labid = l;
			tid = t;
			text = std::string (te);	
		}

		int labid;
		int tid;
		std::string text;
};

class LabelList:public std::vector <LabelEntry>
{
	public:
		LabelList () {}

		void load ();
};

}

#endif //! __RTS2_LABELLIST__

