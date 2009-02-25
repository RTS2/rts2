/* 
 * Driver for OpenTPL mounts.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TELD_IR__
#define __RTS2_TELD_IR__

#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <libnova/libnova.h>

#include "status.h"
#include "telescope.h"

#include <cstdio>
#include <cstdarg>
#include <list>
#include <iostream>

#include "connopentpl.h"

/**
 * Base class for IR telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2TelescopeIr:public Rts2DevTelescope
{
	private:
		std::string ir_ip;
		int ir_port;

		enum { OPENED, OPENING, CLOSING, CLOSED } cover_state;

		void checkErrors ();
		void checkCover ();
		void checkPower ();

		bool doCheckPower;

		void getCover ();
		void initCoverState ();

		std::string errorList;

		Rts2ValueBool *cabinetPower;
		Rts2ValueFloat *cabinetPowerState;

		Rts2ValueDouble *derotatorCurrpos;

		Rts2ValueBool *derotatorPower;

		Rts2ValueDouble *targetDist;
		Rts2ValueDouble *targetTime;

		Rts2ValueDouble *cover;

		Rts2ValueInteger *mountTrack;

		// model values
		Rts2ValueString *model_dumpFile;
		Rts2ValueDouble *model_aoff;
		Rts2ValueDouble *model_zoff;
		Rts2ValueDouble *model_ae;
		Rts2ValueDouble *model_an;
		Rts2ValueDouble *model_npae;
		Rts2ValueDouble *model_ca;
		Rts2ValueDouble *model_flex;

		int infoModel ();

	protected:
		rts2core::OpenTpl *irConn;

		time_t timeout;

		virtual int processOption (int in_opt);

		virtual int initIrDevice ();
		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		Rts2ValueDouble *derotatorOffset;

		int coverClose ();
		int coverOpen ();

		int domeOpen ();
		int domeClose ();

		int setTelescopeTrack (int new_track);

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		bool getDerotatorPower ()
		{
			return derotatorPower->getValueBool ();
		}
	public:
		Rts2TelescopeIr (int argc, char **argv);
		virtual ~ Rts2TelescopeIr (void);
		virtual int ready ();

		virtual int getAltAz ();

		virtual int info ();
		virtual int saveModel ();
		virtual int loadModel ();
		virtual int resetMount ();
};
#endif							 /* !__RTS2_TELD_IR__ */
