/* 
 * Basic camera daemon
 * Copyright (C) 2001-2007 Petr Kubanek <petr@kubanek.net>
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


#ifndef __RTS2_FILTER__
#define __RTS2_FILTER__

namespace rts2camd
{

/**
 * That class is used for filter devices.
 * It's directly attached to camera, so idependent filter devices can
 * be attached to independent cameras.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Filter
{
	public:
		Filter (void)
		{
		}
		virtual ~ Filter (void)
		{
		}
		virtual int init (void) = 0;
		virtual int getFilterNum (void) = 0;
		virtual int setFilterNum (int new_filter) = 0;
};

};

#endif							 /* !__RTS2_FILTER__ */
