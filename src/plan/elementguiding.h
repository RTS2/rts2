/**
 * Script for guiding.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SCRIPT_GUIDING__
#define __RTS2_SCRIPT_GUIDING__

#include "script.h"

namespace rts2script
{

/**
 * Class for guiding.
 *
 * This class takes care of executing guiding script.
 * It works in similar fashion to acquireHAM class - read out image,
 * gets star list, find the brightest star in field, and center on it.
 * You are free to specify start exposure time.
 *
 * @author petr
 */
class ElementGuiding:public Element
{
	public:
		ElementGuiding (Script * in_script, float init_exposure, int in_endSignal);
		virtual ~ ElementGuiding (void);

		virtual void postEvent (Rts2Event * event);

		virtual int nextCommand (Rts2DevClientCamera * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int processImage (Rts2Image * image);
		virtual int waitForSignal (int in_sig);
		virtual void cancelCommands ();
	private:
		float expTime;
		int endSignal;

		int obsId;
		int imgId;

		std::string defaultImgProccess;
		enum
		{
			NO_IMAGE, NEED_IMAGE, WAITING_IMAGE, FAILED
		} processingState;

		// will become -1 in case guiding goes other way then we wanted
		double ra_mult;
		double dec_mult;

		double last_ra;
		double last_dec;

		double min_change;
		double bad_change;

		// this will check sign..
		void checkGuidingSign (double &last, double &mult, double act);
};

}
#endif							 /* !__RTS2_SCRIPT_GUIDING__ */
