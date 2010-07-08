/* 
 * Classes for camera image.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CAMERA_IMAGE__
#define __RTS2_CAMERA_IMAGE__

#include "rts2image.h"
#include <map>
#include <vector>

namespace rts2core
{
class Rts2DevClient;
}

class Rts2DevClientCameraImage;

/**
 * Holds informations about which device waits for which info time.
 */
class ImageDeviceWait
{
	private:
		rts2core::Rts2DevClient * devclient;
		double after;
	public:
		ImageDeviceWait (rts2core::Rts2DevClient * in_devclient, double in_after)
		{
			devclient = in_devclient;
			after = in_after;
		}

		rts2core::Rts2DevClient *getClient () { return devclient; }

		double getAfter () { return after; }
};

/**
 * Holds information about single image.
 */
class CameraImage
{
	public:
		double exStart;
		double exEnd;
		bool dataWriten;
		Rts2Image *image;

		CameraImage (Rts2Image * in_image, double in_exStart)
		{
			image = in_image;
			exStart = in_exStart;
			exEnd = nan ("f");
			dataWriten = false;
		}
		virtual ~ CameraImage (void);

		void waitForDevice (rts2core::Rts2DevClient * devClient, double after);
		bool waitingFor (rts2core::Rts2DevClient * devClient);

		void waitForTrigger (rts2core::Rts2DevClient * devClient) { triggerWaits.push_back (devClient); }
		bool wasTriggered (rts2core::Rts2DevClient * devClient);

		void setExEnd (double in_exEnd) { exEnd = in_exEnd; }

		void setDataWriten () { dataWriten = true; }

		bool canDelete ();
	private:
		std::vector < ImageDeviceWait * > deviceWaits;
		std::vector < rts2core::Rts2DevClient * > triggerWaits;
};

/**
 * That holds images for camd. Images are hold there till all informations are
 * collected, then they are put to processing.
 */
class CameraImages:public std::map <int, CameraImage * >
{
	public:
		CameraImages () {}
		virtual ~CameraImages (void);

		void deleteOld ();

		void infoOK (Rts2DevClientCameraImage * master, rts2core::Rts2DevClient * client);
		void infoFailed (Rts2DevClientCameraImage * master, rts2core::Rts2DevClient * client);
		void wasTriggered (Rts2DevClientCameraImage * master, rts2core::Rts2DevClient * client);
};
#endif							 /* !__RTS2_CAMERA_IMAGE__ */
