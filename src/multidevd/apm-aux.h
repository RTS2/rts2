#ifndef __RTS2_APM_MIRROR__
#define __RTS2_APM_MIRROR__

/**
 * Driver for APM mirror cover
 * Copyright (C) 2015 Stanislav Vitek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sensord.h"
#include "apm-multidev.h"

#define MIRROR_OPENED	5
#define MIRROR_CLOSED	6
#define MIRROR_OPENING	7
#define MIRROR_CLOSING	8

namespace rts2sensord
{

/**
 * APM mirror cover driver.
 *
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMAux : public Sensor, rts2multidev::APMMultidev
{
	public:
		APMAux (const char *name, rts2core::ConnAPM *apmConn, bool hasFan, bool hasBaffle, bool hasRelays, bool hasTemp, bool isControllino, bool opParaller);

		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual void postEvent (rts2core::Event *event);

	protected:
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		rts2core::ValueBool *autoOpen;

		rts2core::ValueSelection *coverState;
		rts2core::ValueBool *fan;
		rts2core::ValueSelection *baffle;
		rts2core::ValueBool *relay1;
		rts2core::ValueBool *relay2;
		rts2core::ValueFloat *temperature;

		// to organize timeouts,..
		rts2core::ValueTime *coverCommand;
		rts2core::ValueTime *baffleCommand;

		enum {NONE, OPENING, CLOSING} commandInProgress;

		bool controllino; // true for Controllino interface - allows for multi ops
		bool paraller; // able to open/close in paraller
	
		int open ();
		int close ();
		int openCover ();
		int closeCover ();
		int openBaffle ();
		int closeBaffle ();

		int relayControl (int n, bool on);

		int sendUDPMessage (const char * _message, bool expectSecond = false);

		/**
		 * Sets Open/Close block bit.
		 */
		void setOCBlock ();
};

}

#endif // __RTS2_APM_MIRROR__
