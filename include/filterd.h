/* 
 * Filter base class.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "device.h"

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
class Filterd:public rts2core::Device
{
	public:
		Filterd (int in_argc, char **in_argv);
		virtual ~ Filterd (void);

		virtual int info ();

		int setFilterNum (rts2core::Connection * conn, int new_filter);

		virtual int homeFilter ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int in_opt);

		virtual int initValues ();
		
		virtual int getFilterNum ();

		/**
		 * Must be called by subclasses, so filter number is updated.
		 */
		virtual int setFilterNum (int new_filter);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		int getFilterNumFromName (std::string filter_name) { return filter->getSelIndex (filter_name); }

		void addFilter (const char *new_filter) { filter->addSelVal (new_filter); }

		/**
		 * Set filter names from space separated argument list.
		 *
		 * @return -1 on error, otherwise 0.
		 */
		int setFilters (char *filters);

	private:
		rts2core::ValueSelection *filter;

		int setFilterNumMask (int new_filter);
};

};
#endif							 /* !__RTS2_FILTERD__ */
