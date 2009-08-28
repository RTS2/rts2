/* 
 * Receive and reacts to Auger showers.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_AUGERSHOOTER__
#define __RTS2_AUGERSHOOTER__

#include "../utilsdb/rts2devicedb.h"
#include "connshooter.h"

#define RTS2_EVENT_AUGER_SHOWER   RTS2_LOCAL_EVENT + 700

namespace rts2grbd
{

class ConnShooter;

class DevAugerShooter:public Rts2DeviceDb
{
	private:
		ConnShooter * shootercnn;
		int port;
		Rts2ValueTime *lastAugerDate;
		Rts2ValueDouble *lastAugerRa;
		Rts2ValueDouble *lastAugerDec;
	protected:
		virtual int processOption (int in_opt);
	public:
		DevAugerShooter (int in_argc, char **in_argv);
		virtual ~ DevAugerShooter (void);

		virtual int ready ()
		{
			return 0;
		}

		virtual int init ();
		int newShower (double lastDate, double ra, double dec);
		bool wasSeen (double lastDate, double ra, double dec);
};

}
#endif							 /*! __RTS2_AUGERSHOOTER__ */
