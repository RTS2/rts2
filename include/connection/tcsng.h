/*
 * TCS NG connection.
 * Copyright (C) 2016 Scott Swindel
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNTCSNG__
#define __RTS2_CONNTCSNG__

#include "connection/tcp.h"

#define NGMAXSIZE    200

#define TCSNG_RA_AZ          0x01
#define TCSNG_DEC_AL         0x02
#define TCSNG_FOCUS          0x04
#define TCSNG_DOME           0x08

namespace rts2core
{

class ConnTCSNG:public ConnTCP
{
	public:
		ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID);

		/**
		 * Request response from TCSNG. Returns pointer to buffer. Throws connection error if request cannot be made.
		 */
		const char * runCommand (const char *cmd, const char *req);
		const char * request (const char *val);
		const char * command (const char *cmd);

		/**
		 * Returns sexadecimal value from telescope control system. Sexadecimal is packet as hhmmss.SS.
		 */
		double getSexadecimalHours (const char *req);
		double getSexadecimalTime (const char *req);
		double getSexadecimalAngle (const char *req);
		void setDebug(bool debug_state){debug=debug_state;};
		bool getDebug(){return debug;};

		int getInteger (const char *req);

		int getReqCount () { return reqCount; }

	private:
		const char *obsID;   // observatory ID
		const char *subID;   // subsystem ID

		char ngbuf[NGMAXSIZE];

		int reqCount;
		bool debug;
};

}

#endif //!__RTS2_CONNTCSNG__
