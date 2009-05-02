/* 
 * Filter base class.
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

#ifndef __RTS2_FILTERD__
#define __RTS2_FILTERD__

#include "../utils/rts2device.h"

/**
 * Filter wheel and related classes.
 */
namespace rts2filterd
{

/**
 * This class is used for filter devices.
 * It's directly attached to camera, so idependent filter devices can
 * be attached to independent cameras.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Filterd:public Rts2Device
{
	private:
		/**
		 * Set filter names from space separated argument list.
		 *
		 * @return -1 on error, otherwise 0.
		 */
		int setFilters (char *filters);
		int setFilterNumMask (int new_filter);
	protected:
		Rts2ValueSelection *filter;

		virtual int processOption (int in_opt);

		virtual int initValues ();

		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Filterd (int in_argc, char **in_argv);
		virtual ~ Filterd (void);

		virtual int info ();

		int setFilterNum (Rts2Conn * conn, int new_filter);

		virtual int homeFilter ();

		virtual int commandAuthorized (Rts2Conn * conn);
};

};
#endif							 /* !__RTS2_FILTERD__ */
