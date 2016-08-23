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

#include "connnosend.h"

#include "augershooter.h"

#define AUGER_BUF_SIZE  20000

// number of cuts
#define NUM_CUTS        5

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
class ConnShooter:public rts2core::ConnNoSend
{
	public:
		ConnShooter (int _port, DevAugerShooter * _master);
		virtual ~ ConnShooter (void);

		virtual int idle ();
		virtual int init ();

		virtual void connectionError (int last_data_size);
		virtual int receive (rts2core::Block *block);

		int lastPacket ();
		double lastTargetTime ();

		void processBuffer (const char *_buf);

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

		enum op_t {CMP_GT, CMP_GE, CMP_EQ, CMP_LE, CMP_LT};

		std::list < std::string > failedCuts[NUM_CUTS];
		int cutindex;

		std::string failedCutsString (int i);

		template <class c> bool compare (c p1, op_t op, c p2, const char * pname)
		{
			bool ret = false;
			static const char *op_c[] = {">", ">=", "==", "<=", "<"};
			switch (op)
			{
				case CMP_GT:
					ret = p1 > p2;
					break;
				case CMP_GE:
					ret = p1 >= p2;
					break;
				case CMP_EQ:
					ret = p1 == p2;
					break;
				case CMP_LE:
					ret = p1 <= p2;
					break;
				case CMP_LT:
					ret = p1 < p2;
					break;
			}
			if (ret == false)
			{
				std::ostringstream os;
				os << pname << ": " << p1 << op_c[op] << p2 << " ";
				failedCuts[cutindex].push_back (os.str ());
			}
			return ret;
		}
};

}
#endif							 /* !__RTS2_SHOOTERCONN__ */
