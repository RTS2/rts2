/* 
 * Client which produces images.
 * Copyright (C) 2003-2007,2011 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVCLIENT_IMG__
#define __RTS2_DEVCLIENT_IMG__

#include "rts2object.h"
#include "rts2devclient.h"
#include "rts2command.h"

#include "image.h"
#include "cameraimage.h"

#include <libnova/libnova.h>

namespace rts2image
{

typedef enum
{ IMAGE_DO_BASIC_PROCESSING, IMAGE_KEEP_COPY }
imageProceRes;

/**
 * Defines client descendants capable to stream themselves
 * to an Image.
 *
 */
class DevClientCameraImage:public rts2core::Rts2DevClientCamera
{
	public:
		DevClientCameraImage (Rts2Conn * in_connection, std::string templateFile = std::string (""));
		virtual ~DevClientCameraImage (void);
		virtual void postEvent (Rts2Event * event);

		virtual void newDataConn (int data_conn);
		virtual void fullDataReceived (int data_conn, rts2core::DataChannels *data);
		virtual Image *createImage (const struct timeval *expStart);
		virtual void beforeProcess (Image * image);

		void processCameraImage (CameraImages::iterator cis);
		virtual void stateChanged (Rts2ServerState * state);
 
		void setSaveImage (int in_saveImage) { saveImage = in_saveImage; }

		/**
		 * Returns image on top of the image queue.
		 */
		Image *getActualImage () { return lastImage; }

	protected:

		/**
		 * Called before processImage, as soon as data becomes available.
		 */
		virtual void cameraImageReady (Image *image) {}

		/**
		 * This function carries image processing.  Based on the return value, image
		 * will be deleted when new image is taken, or deleting of the image will
		 * become responsibility of process which forked with this call.
		 *
		 * @return IMAGE_DO_BASIC_PROCESSING when image still should be handled by
		 * connection, or IMAGE_KEEP_COPY if processing instance will delete image.
		 */
		virtual imageProceRes processImage (Image * image);

		void clearImages ()
		{
			for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
			{
				delete (*iter).second;
			}
			images.clear ();
		}

		Image *setImage (Image * old_img, Image * new_image);

		int getExposureNumber () { return ++expNum; }

		int chipNumbers;
		int saveImage;

		// some camera characteristics..
		double xoa;
		double yoa;
		double ter_xoa;
		double ter_yoa;
		std::string instrume;
		std::string telescop;
		std::string origin;

		virtual void exposureStarted ();
		virtual void exposureEnd ();

		bool waitForMetaData ();
		void setTriggered () { triggered = true; }

	private:
		void writeFilter (Image *img);

		// we have to allocate that field as soon as we get the knowledge of
		// camera chip numbers..
		CameraImages images;

		// current image
		CameraImage *actualImage;

		// last image
		Image *lastImage;

		// number of exposure
		int expNum;

		bool triggered;

		// already received informations from those devices..
		std::vector < rts2core::Rts2DevClient * > prematurelyReceived;

		// template for headers.
		Rts2ConfigRaw *fitsTemplate;
};

class DevClientTelescopeImage:public rts2core::Rts2DevClientTelescope
{
	public:
		DevClientTelescopeImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);
		/**
		 * Get target coordinates.
		 */
		void getEqu (struct ln_equ_posn *tel);
		int getMountFlip ();
		void getObs (struct ln_lnlat_posn *obs);
		double getDistance (struct ln_equ_posn *in_pos);
};

class DevClientFocusImage:public rts2core::Rts2DevClientFocus
{
	public:
		DevClientFocusImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);
};

class DevClientWriteImage:public rts2core::Rts2DevClient
{
	public:
		DevClientWriteImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);

		virtual void infoOK ();
		virtual void infoFailed ();

		virtual void stateChanged (Rts2ServerState * state);
};

class Rts2CommandQueImage:public rts2core::Rts2Command
{
	public:
		Rts2CommandQueImage (rts2core::Block * in_owner, Image * image);
};

class Rts2CommandQueObs:public rts2core::Rts2Command
{
	public:
		Rts2CommandQueObs (rts2core::Block * in_owner, int in_obsId);
};

}
#endif							 /* !__RTS2_DEVCLIENT_IMG__ */
