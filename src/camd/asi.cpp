/* 
 * Driver for ASI cameras.
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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

#include "camd.h"
#include "ASICamera2.h"

namespace rts2camd
{

/**
 * Class for ASI camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ASI:public Camera
{
	public:
		ASI (int in_argc, char **in_argv);
		virtual ~ASI (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initChips ();
		virtual int info ();
		virtual int startExposure ();
		virtual int doReadout ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		rts2core::ValueInteger *cameraId;
};

};

using namespace rts2camd;

ASI::ASI (int in_argc, char **in_argv):Camera(in_argc, in_argv)
{
	createValue(cameraId, "cameraid", "camera ID", false);

	addOption('p', NULL, 1, "ASI camera ID");
}

ASI::~ASI ()
{
	ASICloseCamera(cameraId->getValueInteger());
}

int ASI::processOption(int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			cameraId->setValueCharArr(optarg);
			break;
		default:
			return Camera::processOption(in_opt);
	}
}

int ASI::initHardware ()
{
	ASI_ERROR_CODE ret = ASIOpenCamera(cameraId->getValueInteger());
	if (ret != ASI_SUCCESS)
		return ret;
	ret = ASIInitCamera(cameraId->getValueInteger());
	return ret == ASI_SUCCESS ? 0 : -2;
}

int ASI::initChips ()
{
	return 0;
}

int ASI::info ()
{
	return Camera::info();
}

int ASI::startExposure ()
{
	return -2;
}

int ASI::doReadout ()
{
	return -2;
}

int ASI::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	return Camera::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	ASI device (argc, argv);
	return device.run ();
}
