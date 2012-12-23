/*
 * Script for acqustion of star.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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


#ifndef __RTS2_SCRIPT_ELEMENT_ACQUIRE__
#define __RTS2_SCRIPT_ELEMENT_ACQUIRE__

#include "rts2script/script.h"
#include "rts2script/element.h"

namespace rts2script
{

class ElementAcquire:public Element
{
	public:
		ElementAcquire (Script * in_script, double in_precision, float in_expTime, struct ln_equ_posn *in_center_pos);
		virtual void postEvent (rts2core::Event * event);
		virtual int nextCommand (rts2core::DevClientCamera * camera, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int processImage (rts2image::Image * image);
		virtual void cancelCommands ();

		virtual double getExpectedDuration (int runnum);

		virtual void printXml (std::ostream &os) { os << "  <acquire length='" << expTime << "' precision='" << reqPrecision << "'/>"; }
		virtual void printScript (std::ostream &os) { os << COMMAND_ACQUIRE << " " << reqPrecision << " " << expTime; }
		virtual void printJson (std::ostream &os) { os << "\"cmd\":\"" << COMMAND_ACQUIRE << "\",\"duration\":" << expTime << ",\"precision\":" << reqPrecision; }
	protected:
		double reqPrecision;
		double lastPrecision;
		float expTime;

		enum { NEED_IMAGE, WAITING_IMAGE, WAITING_ASTROMETRY, WAITING_MOVE, PRECISION_OK, PRECISION_BAD, FAILED} processingState;

		std::string defaultImgProccess;
		int obsId;
		int imgId;
		struct ln_equ_posn center_pos;
};

}
#endif							 /* !__RTS2_SCRIPT_ELEMENT_ACQUIRE__ */
