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

#ifndef __RTS2_SHOOTERCONN__
#define __RTS2_SHOOTERCONN__

#include "../utils/rts2connnosend.h"

#include "augershooter.h"

#define AUGER_BUF_SIZE  5000

namespace rts2grbd
{

class DevAugerShooter;

/**
 * Connection for shooter. Wait on socket from shooter for new requests. If new
 * request arrive, it is properly dispatched through database and passed to 
 * executor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnShooter:public Rts2ConnNoSend
{
	public:
		ConnShooter (int _port, DevAugerShooter * _master);
		virtual ~ ConnShooter (void);

		virtual int idle ();
		virtual int init ();

		virtual void connectionError (int last_data_size);
		virtual int receive (fd_set * set);

		int lastPacket ();
		double lastTargetTime ();

	private:
		DevAugerShooter *master;

		struct timeval last_packet;

		double last_target_time;

		// init server socket
		int init_listen ();

		void getTimeTfromGPS (long GPSsec, long GPSusec, double &out_time);

		int port;

		char nbuf[AUGER_BUF_SIZE];
		// position inside network buffer
		int nbuf_pos;

		int processAuger ();
		

		// last_auger_xxx are used in processAuger and receive methods
		double last_auger_date;
		double last_auger_ra;
		double last_auger_dec;
};

}
#endif							 /* !__RTS2_SHOOTERCONN__ */
