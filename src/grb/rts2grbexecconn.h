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

#ifndef __RTS2_GRBEXECCONN__
#define __RTS2_GRBEXECCONN__

#include "../utils/connfork.h"

// executes GRB programm..
class Rts2GrbExecConn:public rts2core::ConnFork
{
	private:
		char **argvs;
	public:
		Rts2GrbExecConn (Rts2Block * in_master, char *execFile, int in_tar_id,
			int in_grb_id, int in_grb_seqn, int in_grb_type,
			double in_grb_ra, double in_grb_dec, bool in_grb_is_grb,
			time_t * in_grb_date, float in_grb_errorbox, int grb_isnew);
		virtual ~ Rts2GrbExecConn (void);

		virtual int newProcess ();
};
#endif							 /* !__RTS2_GRBEXECCONN__ */
