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
class APMMirror : public Sensor, rts2multidev::APMMultidev
{
	public:
		APMMirror (const char *name, rts2core::ConnAPM *apmConn);
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);
		virtual int info ();

	private:
		int mirror_state;
                rts2core::ValueString *mirror;
                int open ();
                int close ();
                int sendUDPMessage (const char * _message);
};

}

#endif // __RTS2_APM_MIRROR__
