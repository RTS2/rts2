/* 
 * Class for GPIB sensors.
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

#ifndef __RTS2_SENSOR_GPIB__
#define __RTS2_SENSOR_GPIB__

#include "sensord.h"

#include <gpib/ib.h>

namespace rts2sensord
{

class Gpib:public Sensor
{
	private:
		int minor;
		int pad;

		int gpib_dev;
	protected:
		int gpibWrite (const char *buf);
		int gpibRead (void *buf, int blen);
		int gpibWriteRead (const char *buf, char *val, int blen = 50);

		int gpibWaitSRQ ();

		virtual int processOption (int in_opt);
		virtual int init ();

		void setPad (int in_pad)
		{
			pad = in_pad;
		}
	public:
		Gpib (int in_argc, char **in_argv);
		virtual ~ Gpib (void);
};

};
#endif							 /* !__RTS2_SENSOR_GPIB__ */
