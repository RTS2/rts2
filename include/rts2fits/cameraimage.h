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

#include "rts2fits/image.h"

#include <map>
#include <vector>

namespace rts2core
{
class DevClient;
}

namespace rts2image
{

class DevClientCameraImage;



/**
 * Holds informations about which device waits for which info time.
 */
class ImageDeviceWait
{
	public:
		ImageDeviceWait (rts2core::DevClient * in_devclient, double in_after)
		{
			devclient = in_devclient;
			after = in_after;
		}

		rts2core::DevClient *getClient () { return devclient; }

		double getAfter () { return after; }

	private:
		rts2core::DevClient * devclient;
		double after;
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
		Image *image;

		CameraImage (Image * in_image, double in_exStart, std::vector < rts2core::DevClient * > &_prematurelyReceived)
		{
			image = in_image;
			exStart = in_exStart;
			exEnd = NAN;
			dataWriten = false;
			prematurelyReceived = _prematurelyReceived;
		}
		virtual ~ CameraImage (void);

		void waitForDevice (rts2core::DevClient * devClient, double after);
		bool waitingFor (rts2core::DevClient * devClient);

		void waitForTrigger (rts2core::DevClient * devClient);
		bool wasTriggered (rts2core::DevClient * devClient);

		void setExEnd (double in_exEnd) { exEnd = in_exEnd; }

		void writeMetaData (struct imghdr *im_h) { image->writeMetaData (im_h); }

		void writeData (char *_data, char *_fullTop, int nchan) { image->writeData (_data, _fullTop, nchan); dataWriten = true; }

		bool canDelete ();

		/**
		 * Return true if the image is waiting for some of the metadata.
		 */
		bool waitForMetaData ();
	private:
		std::vector < ImageDeviceWait * > deviceWaits;
		std::vector < rts2core::DevClient * > triggerWaits;
		std::vector < rts2core::DevClient * > prematurelyReceived;
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

		void infoOK (DevClientCameraImage * master, rts2core::DevClient * client);
		void infoFailed (DevClientCameraImage * master, rts2core::DevClient * client);
		bool wasTriggered (DevClientCameraImage * master, rts2core::DevClient * client);
};

}
#endif							 /* !__RTS2_CAMERA_IMAGE__ */
