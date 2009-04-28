/* 
 * Client which produces images.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/rts2object.h"
#include "../utils/rts2devclient.h"
#include "../utils/rts2command.h"

#include "rts2image.h"
#include "cameraimage.h"

#include <libnova/libnova.h>

typedef enum
{ IMAGE_DO_BASIC_PROCESSING, IMAGE_KEEP_COPY }
imageProceRes;

/**
 * Defines client descendants capable to stream themselves
 * to an Rts2Image.
 *
 */
class Rts2DevClientCameraImage:public Rts2DevClientCamera
{
	private:
		void writeFilter (Rts2Image *img);

		// we have to allocate that field as soon as we get the knowledge of
		// camera chip numbers..
		CameraImages images;

		// current image
		CameraImage *actualImage;

		// last image
		Rts2Image *lastImage;

		// number of exposure
		int expNum;

	protected:

		/**
		 * Returns image on top of the que.
		 */
		Rts2Image *getActualImage ()
		{
			return lastImage;
		}

		void clearImages ()
		{
			for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
			{
				delete (*iter).second;
			}
			images.clear ();
		}

		Rts2Image *setImage (Rts2Image * old_img, Rts2Image * new_image);

		int getExposureNumber ()
		{
			return ++expNum;
		}

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
	public:
		Rts2DevClientCameraImage (Rts2Conn * in_connection);
		virtual ~Rts2DevClientCameraImage (void);
		virtual void postEvent (Rts2Event * event);

		virtual void newDataConn (int data_conn);
		virtual void fullDataReceived (int data_conn, Rts2DataRead *data);
		virtual Rts2Image *createImage (const struct timeval *expStart);
		virtual void beforeProcess (Rts2Image * image);

		/**
		 * This function carries image processing.  Based on the return value, image
		 * will be deleted when new image is taken, or deleting of the image will
		 * become responsibility of process which forked with this call.
		 *
		 * @return IMAGE_DO_BASIC_PROCESSING when image still should be handled by
		 * connection, or IMAGE_KEEP_COPY if processing instance will delete image.
		 */
		virtual imageProceRes processImage (Rts2Image * image);

		void processCameraImage (CameraImages::iterator cis);

		void setSaveImage (int in_saveImage)
		{
			saveImage = in_saveImage;
		}
};

class Rts2DevClientTelescopeImage:public Rts2DevClientTelescope
{
	public:
		Rts2DevClientTelescopeImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);
		void getEqu (struct ln_equ_posn *tel);
		void getEquTel (struct ln_equ_posn *tel);
		void getEquTar (struct ln_equ_posn *tar);
		int getMountFlip ();
		void getObs (struct ln_lnlat_posn *obs);
		double getDistance (struct ln_equ_posn *in_pos);
};

class Rts2DevClientFocusImage:public Rts2DevClientFocus
{
	public:
		Rts2DevClientFocusImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientWriteImage:public Rts2DevClient
{
	public:
		Rts2DevClientWriteImage (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);

		virtual void infoOK ();
		virtual void infoFailed ();
};

class Rts2CommandQueImage:public Rts2Command
{
	public:
		Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image);
};

class Rts2CommandQueObs:public Rts2Command
{
	public:
		Rts2CommandQueObs (Rts2Block * in_owner, int in_obsId);
};
#endif							 /* !__RTS2_DEVCLIENT_IMG__ */
