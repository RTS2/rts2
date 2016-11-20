/**
 * Container for multiple APM devices.
 * Copyright (C) 2016 Petr Kubanek, <petr@kubanek.net>
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

#ifndef __RTS2_APM_MULTIDEV__
#define __RTS2_APM_MULTIDEV__

#include "multidev.h"
#include "connection/apm.h"

namespace rts2multidev
{

class APMMultidev
{
	public:
		APMMultidev (rts2core::ConnAPM *conn)
		{
			apmConn = conn;
		}

		rts2core::ConnAPM *apmConn;
};

class APMMultiBase:public rts2core::MultiBase
{
	public:
		APMMultiBase (int argc, char **argv);
		virtual ~APMMultiBase ();

		virtual int run ();

	protected:
		virtual int processOption (int opt);

		virtual int initHardware ();
	private:
		const char *filterName;
		const char *auxName;

		const char *filters;

		bool hasFan;
		bool hasBaffle;
		bool hasRelays;
		bool hasTemp;

		HostString *host;
		rts2core::ConnAPM *apmConn;

		rts2core::MultiDev md;
};

}

#endif // !__RTS2_APM_MULTIDEV__
