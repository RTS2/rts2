/* 
 * Class for devices with scripting support.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SCRIPTDEVICE__
#define __RTS2_SCRIPTDEVICE__

#include "device.h"

namespace rts2core
{

/**
 * Base class for devices which can run scripts. This class holds variables
 * which can describe script and its progress. The variables hold running script,
 * and values which described which command is actually running.
 *   
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ScriptDevice:public Device
{
	public:
		ScriptDevice (int in_argc, char **in_argv, int in_device_type, const char *default_name);

		virtual int commandAuthorized (Rts2Connection * conn);

	private:
		rts2core::ValueInteger * scriptRepCount;
		rts2core::ValueString *runningScript;
		rts2core::ValueString *scriptComment;
		rts2core::ValueInteger *commentNumber;
		rts2core::ValueInteger *scriptPosition;
		rts2core::ValueInteger *scriptLen;
		rts2core::ValueSelection *scriptStatus;
		rts2core::IntegerArray *elementPosition;

		void blockEnter ();
		void blockExit ();
		void elementExecuted ();
		void elementFirst ();
};

}
#endif							 /* !__RTS2_SCRIPTDEVICE__ */
