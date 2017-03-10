/* 
 * Master for Sitech rotators.
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

#ifndef __RTS2_SITECH_MULTI__
#define __RTS2_SITECH_MULTI__

#include "connection/sitech.h"
#include "constsitech.h"
#include "sitech-rotator.h"
#include "multidev.h"


#define NUM_ROTATORS      2

namespace rts2rotad
{

class SitechRotator;

class SitechMulti:public rts2core::MultiBase
{
	public:
		SitechMulti (int argc, char **argv);
		virtual ~SitechMulti ();

		virtual int run ();
		
		int callInfo ();
		void callUpdate ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual bool isRunning (rts2core::Rts2Connection *conn) { return false; }
		virtual rts2core::Rts2Connection *createClientConnection (rts2core::NetworkAddress * in_addr) { return NULL; }

	private:
		void derSetTarget ();
		
		const char *der_tty;
		rts2core::ConnSitech *derConn;

		const char *derdefaults[2];

		SitechRotator *rotators[NUM_ROTATORS];

		rts2core::MultiDev md;

		rts2core::SitechAxisStatus der_status;
		rts2core::SitechYAxisRequest der_Yrequest;
		rts2core::SitechXAxisRequest der_Xrequest;

		uint8_t xbits;
		uint8_t ybits;
};

}

#endif //!__RTS2_SITECH_MULTI__
