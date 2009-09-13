/* 
 * Receives informations from GCN via socket, put them to database.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_GRBD__
#define __RTS2_GRBD__

#include "../utilsdb/rts2devicedb.h"
#include "conngrb.h"
#include "rts2grbfw.h"

// when we get GRB packet..
#define RTS2_EVENT_GRB_PACKET      RTS2_LOCAL_EVENT + 600
#define EVENT_TIMER_GCNCNN_INIT    RTS2_LOCAL_EVENT + 601

namespace rts2grbd
{

class ConnGrb;

/**
 * Receive info from GCN via socket, put them to DB.
 *
 * Based on http://gcn.gsfc.nasa.gov/socket_demo.c
 * socket_demo     Ver: 3.32   29 May 06
 * which is in this directory. Only "active" satellite packets are processed.
 *
 * If new version of socket_demo.c show up, we need to invesigate
 * modifications to include it.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Grbd:public Rts2DeviceDb
{
	private:
		ConnGrb * gcncnn;
		char *gcn_host;
		int gcn_port;
		int do_hete_test;

		int forwardPort;

		char *addExe;
		int execFollowups;

		Rts2ValueBool *grb_enabled;

		Rts2ValueTime *last_packet;
		Rts2ValueDouble *delta;
		Rts2ValueString *last_target;
		Rts2ValueTime *last_target_time;
		Rts2ValueRaDec *last_target_radec;
		Rts2ValueBool *execConnection;

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int init ();
		virtual void help ();

		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);
	public:
		Grbd (int argc, char **argv);
		virtual ~ Grbd ();

		virtual int ready ()
		{
			return 0;
		}
		virtual int info ();
		virtual void postEvent (Rts2Event * event);

		int newGcnGrb (int tar_id);

		virtual int commandAuthorized (Rts2Conn * conn);
};

}
#endif							 /* __RTS2_GRBD__ */
