/* 
 * Class for GPIB sensors.
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

#ifndef __RTS2_SENSOR_GPIB__
#define __RTS2_SENSOR_GPIB__

#include "sensord.h"
#include "conngpib.h"
#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Provides basic GPIB functionalities. Should be used by all GPIB enabled
 * devices to facilite communication over GPIB bus.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Gpib:public Sensor
{
	public:
		Gpib (int argc, char **argv);
		virtual ~ Gpib (void);

	protected:
		void gpibWrite (const char *cmd) { connGpib->gpibWrite (cmd); }
		void gpibWriteBuffer (const char *cmd, int len) { connGpib->gpibWriteBuffer (cmd, len); }
		void gpibRead (void *reply, int &blen) { connGpib->gpibRead (reply, blen); }
		void gpibWriteRead (const char *cmd, char *reply, int blen) { connGpib->gpibWriteRead (cmd, reply, blen); }

		/**
		 * Write value to GPIB bus. Except for Boolean values, getDisplayValue function is
		 * used to write value to the bus. Boolean values are written
		 * using either ON or OFF as value.
		 *
		 * @param name   GPIB value name.
		 * @param value  rts2core::Value class.
		 *
		 * @throw rts2core::Error and its descendants.
		 */
		void writeValue (const char *name, rts2core::Value *value);

		void readInt (const char *buf, int &val) { connGpib->readInt (buf, val); }

		void readValue (const char *buf, rts2core::Value *val) { connGpib->readValue (buf, val); }

		void readValue (const char *subsystem, std::list < rts2core::Value * >&vals, int prefix_num) { connGpib->readValue (subsystem, vals, prefix_num); }

		void readValue (const char *buf, rts2core::ValueString * val) { connGpib->readValue (buf, val); }

		void readValue (const char *buf, rts2core::ValueDouble * val) { connGpib->readValue (buf, val); }

		void readValue (const char *buf, rts2core::ValueFloat * val) { connGpib->readValue (buf, val); }

		void readValue (const char *buf, rts2core::ValueBool * val) { connGpib->readValue (buf, val); }

		void readValue (const char *buf, rts2core::ValueSelection * val) { connGpib->readValue (buf, val); }

		void gpibWaitSRQ () { connGpib->gpibWaitSRQ (); }

		void devClear () { connGpib->devClear (); }

		void settmo (float _sec) { connGpib->settmo (_sec); }

		virtual int processOption (int in_opt);
		virtual int initValues ();
		virtual int initHardware ();
	
	private:
		int pad;
		int minor;

		// used only for GPIB enet interface
		HostString *enet_addr;
		const char *prologix;
		const char *serial_port;

		rts2core::bSpeedT serial_baud;
		rts2core::cSizeT serial_csize;
		rts2core::parityT serial_parity;
		const char *serial_sep;

		ConnGpib *connGpib;
		
		bool debug;
};

};
#endif		 /* !__RTS2_SENSOR_GPIB__ */
