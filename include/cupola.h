/* 
 * Copula driver skeleton.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_COPULA__
#define __RTS2_COPULA__

#include "dome.h"
#include "status.h"

#include <libnova/libnova.h>

using namespace rts2dome;

namespace rts2dome
{

/**
 * Abstract class for cupola. Cupola have slit which can be opened and can
 * rotate in azimuth to allow telescope see through it.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Cupola:public Dome
{
	public:
		Cupola (int argc, char **argv, bool inhibit_auto_close = false);

		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();
		virtual int idle ();

		int moveTo (rts2core::Connection * conn, double ra, double dec);
		virtual int moveStop ();

		// returns target current alt & az
		void getTargetAltAz (struct ln_hrz_posn *hrz);

		/**
		 * Called to see if copula need to change its position. Called repeatibly from idle call.
		 * 
		 * @return false when we are satisfied with curent position, true when split position change is needed.
		 */
		virtual bool needSlitChange ();
		// calculate split width in arcdeg for given altititude; when copula don't have split at given altitude, returns -1
		virtual double getSlitWidth (double alt) = 0;

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		// called to bring copula in sync with target az
		virtual int moveStart ();
		// same meaning of return values as Rts2DevTelescope::isMoving
		virtual long isMoving ()
		{
			return -2;
		}
		virtual int moveEnd ();

		void setTargetAz (double in_az) { tarAltAz->setAz (in_az); }
		double getTargetAz () { return tarAltAz->getAz (); }

		void setCurrentAz (double in_az) { currentAz->setValueDouble (in_az); }
		double getCurrentAz () { return currentAz->getValueDouble (); }

		double getTargetDistance () { return targetDistance->getValueDouble (); }

		double getTargetRa () { return tarRaDec->getRa (); }
		double getTargetDec () { return tarRaDec->getDec (); }
		struct ln_lnlat_posn *getObserver ()
		{
			return observer;
		}
		void synced ();

	private:
		// defaults to 0, 0; will be readed from config file
		struct ln_lnlat_posn *observer;

		rts2core::ValueRaDec *tarRaDec;
		rts2core::ValueAltAz *tarAltAz;
		rts2core::ValueDouble *currentAz;

		rts2core::ValueDouble *targetDistance;

		rts2core::ValueBool *trackTelescope;
		rts2core::ValueBool *trackDuringDay;

		rts2core::ValueFloat *parkAz;

		char *configFile;
};

}

#endif							 /* !__RTS2_COPULA__ */
