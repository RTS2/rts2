/* 
 * Skeleton for applications which works with targets.
 * Copyright (C) 2006-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGETAPP__
#define __RTS2_TARGETAPP__

#include <libnova/libnova.h>

#include "rts2db/appdb.h"
#include "rts2db/target.h"

/**
 * Base of the target application.
 *
 * Provides routines for asking for target and resolving its name using Simbad.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2TargetApp:public rts2db::AppDb
{
	public:
		Rts2TargetApp (int argc, char **argv);
		virtual ~ Rts2TargetApp (void);

		virtual int init ();

	protected:
		/**
		 * Pointer to Target structure holding actual target.
		 */
		rts2db::Target * target;
		struct ln_lnlat_posn *obs;

		/**
		 * Tries to resolve object from provided string.
		 */
		void getObject (std::string obj_text);

		int askForDegrees (const char *desc, double &val);
		int askForObject (const char *desc, std::string obj_text = std::string (""));
};
#endif							 /* !__RTS2_TARGETAPP__ */
