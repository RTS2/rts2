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

#include <ctype.h>

#include "rts2devcliimg.h"
#include "../utils/rts2config.h"
#include "../utils/timestamp.h"

Rts2DevClientCameraImage::Rts2DevClientCameraImage (Rts2Conn * in_connection):Rts2DevClientCamera
(in_connection)
{
	chipNumbers = 0;
	saveImage = 1;

	Rts2Config *config = Rts2Config::instance ();

	xplate = 1;
	yplate = 1;
	xoa = 0;
	yoa = 0;
	ter_xoa = nan ("f");
	ter_yoa = nan ("f");
	flip = 1;

	config->getDouble (connection->getName (), "xplate", xplate);
	config->getDouble (connection->getName (), "yplate", yplate);
	config->getDouble (connection->getName (), "xoa", xoa);
	config->getDouble (connection->getName (), "yoa", yoa);
	config->getDouble (connection->getName (), "ter_xoa", ter_xoa);
	config->getDouble (connection->getName (), "ter_yoa", ter_yoa);
	config->getInteger (connection->getName (), "flip", flip);

	telescop[0] = '\0';
	instrume[0] = '\0';
	origin[0] = '\0';

	config->getString (connection->getName (), "instrume", instrume);
	config->getString (connection->getName (), "telescop", telescop);
	config->getString (connection->getName (), "origin", origin);

	actualImage = NULL;
	lastImage = NULL;

	expNum = 0;
}


Rts2DevClientCameraImage::~Rts2DevClientCameraImage (void)
{
}


Rts2Image *
Rts2DevClientCameraImage::setImage (Rts2Image * old_img,
Rts2Image * new_image)
{
	for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		CameraImage *ci = (*iter).second;
		if (ci->image == old_img)
		{
			ci->image = new_image;
			return new_image;
		}
	}
	return NULL;
}


void
Rts2DevClientCameraImage::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_INFO_DEVCLI_OK:
			images.infoOK (this, (Rts2DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((Rts2DevClient *) event->getArg ());
			break;
		case EVENT_INFO_DEVCLI_FAILED:
			images.infoFailed (this, (Rts2DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((Rts2DevClient *) event->getArg ());
			break;
		case EVENT_NUMBER_OF_IMAGES:
			*((int *)event->getArg ()) += images.size ();
			if (actualImage)
				*((int *)event->getArg ()) += 1;
			break;
	}
	Rts2DevClientCamera::postEvent (event);
}


void
Rts2DevClientCameraImage::writeFilter (Rts2Image *img)
{
	int camFilter = img->getFilterNum ();
	char imageFilter[4];
	strncpy (imageFilter, getConnection()->getValueSelection ("filter", camFilter), 4);
	imageFilter[4] = '\0';
	img->setFilter (imageFilter);
}


void
Rts2DevClientCameraImage::newDataConn (int data_conn)
{
	if (!actualImage)
	{
		logStream (MESSAGE_ERROR) << "Data stream started without exposure!" << sendLog;
		return;
	}
	images[data_conn] = actualImage;
	actualImage = NULL;
}


void
Rts2DevClientCameraImage::fullDataReceived (int data_conn, Rts2DataRead *data)
{
	CameraImages::iterator iter = images.find (data_conn);
	if (iter != images.end ())
	{
		CameraImage *ci = (*iter).second;
		ci->image->writeDate (data->getDataBuff (), data->getDataTop ());
		ci->setDataWriten ();
		if (ci->canDelete ())
		{
			processCameraImage (iter++);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "getData, but not all metainfo - size of images:" << images.size () << sendLog;
		}
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "getTopImage is NULL" << sendLog;
	}
}


Rts2Image *
Rts2DevClientCameraImage::createImage (const struct timeval *expStart)
{
	return new Rts2Image ("%c_%y%m%d-%H%M%S-%s.fits", getExposureNumber (), expStart, connection);
}


void
Rts2DevClientCameraImage::processCameraImage (CameraImages::iterator cis)
{
	CameraImage *ci = (*cis).second;
	// create new image of requsted type
	beforeProcess (ci->image);
	if (saveImage)
	{
		writeFilter (ci->image);
		// set filter..
		// save us to the disk..
		ci->image->saveImage ();
	}
	// do basic processing
	imageProceRes res = processImage (ci->image);
	if (res == IMAGE_KEEP_COPY)
	{
		setImage (ci->image, NULL);
	}
	// remove us
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Erase image " << ci << sendLog;
	#endif						 /* DEBUG_EXTRA */
	if (lastImage == ci->image)
		lastImage = NULL;
	delete ci;
	images.erase (cis);
}


void
Rts2DevClientCameraImage::beforeProcess (Rts2Image * image)
{
}


imageProceRes Rts2DevClientCameraImage::processImage (Rts2Image * image)
{
	return IMAGE_DO_BASIC_PROCESSING;
}


void
Rts2DevClientCameraImage::exposureStarted ()
{
	double exposureTime = getConnection ()->getValueDouble ("exposure");
	struct timeval expStart;
	const char *focuser;
	gettimeofday (&expStart, NULL);
	Rts2Image *image = createImage (&expStart);
	if (image == NULL)
		return;
	image->setExposureLength (exposureTime);

	image->setValue ("XPLATE", xplate,
		"xplate (scale in X axis; divide by binning (BIN_H)!)");
	image->setValue ("YPLATE", yplate,
		"yplate (scale in Y axis; divide by binning (BIN_V)!)");

	image->setCameraName (getName ());
	image->setInstrument (instrume.c_str ());
	image->setTelescope (telescop.c_str ());
	image->setOrigin (origin.c_str ());

	image->setEnvironmentalValues ();

	if (image->getTargetType () == TYPE_TERESTIAL
		&& !isnan (ter_xoa) && !isnan (ter_yoa))
	{
		image->setXoA (ter_xoa);
		image->setYoA (ter_yoa);
	}
	else
	{
		image->setXoA (xoa);
		image->setYoA (yoa);
	}
	image->setValue ("FLIP", flip,
		"camera flip (since most astrometry devices works as mirrors");
	focuser = getConnection ()->getValueChar ("focuser");
	if (focuser)
	{
		image->setFocuserName (focuser);
	}
	// delete old image
	delete actualImage;
	// create image
	actualImage = new CameraImage (image, getMaster ()->getNow ());
	actualImage->image->writeConn (getConnection (), EXPOSURE_START);

	lastImage = image;
	connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE, actualImage));
	Rts2DevClientCamera::exposureStarted ();
}


void
Rts2DevClientCameraImage::exposureEnd ()
{
	logStream (MESSAGE_DEBUG) << "exposureEnd " << connection->getName () << sendLog;

	if (actualImage)
	{
		actualImage->setExEnd (getMaster ()->getNow ());
		connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE_ENDS, actualImage));
	}

	Rts2DevClientCamera::exposureEnd ();
}


Rts2DevClientTelescopeImage::Rts2DevClientTelescopeImage (Rts2Conn * in_connection):Rts2DevClientTelescope
(in_connection)
{
}


void
Rts2DevClientTelescopeImage::postEvent (Rts2Event * event)
{
	struct ln_equ_posn *change;	 // change in degrees
	CameraImage * ci;
	switch (event->getType ())
	{
		case EVENT_WRITE_TO_IMAGE:
			Rts2Image *image;
			struct ln_equ_posn object;
			struct ln_lnlat_posn obs;
			struct ln_equ_posn suneq;
			struct ln_hrz_posn sunhrz;
			double infotime;
			ci = (CameraImage *) event->getArg ();
			image = ci->image;
			image->setMountName (connection->getName ());
			getEqu (&object);
			getObs (&obs);
			image->writeConn (getConnection (), EXPOSURE_START);
			infotime = getConnection ()->getValueDouble ("infotime");
			image->setValue ("MNT_INFO", infotime,
				"time when mount informations were collected");

			image->setMountFlip (getMountFlip ());
			ln_get_solar_equ_coords (image->getExposureJD (), &suneq);
			ln_get_hrz_from_equ (&suneq, &obs, image->getExposureJD (), &sunhrz);
			image->setValue ("SUN_ALT", sunhrz.alt, "solar altitude");
			image->setValue ("SUN_AZ", sunhrz.az, "solar azimuth");
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *)event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
		case EVENT_GET_RADEC:
			getEqu ((struct ln_equ_posn *) event->getArg ());
			break;
		case EVENT_MOUNT_CHANGE:
			change = (struct ln_equ_posn *) event->getArg ();
			queCommand (new Rts2CommandChange (this, change->ra, change->dec));
			break;
	}

	Rts2DevClientTelescope::postEvent (event);
}


void
Rts2DevClientTelescopeImage::getEqu (struct ln_equ_posn *tel)
{
	Rts2ValueRaDec *vradec = (Rts2ValueRaDec *) getConnection ()->getValue ("TAR");
	if (!vradec)
		return;

	tel->ra = vradec->getRa ();
	tel->dec = vradec->getDec ();
}


void
Rts2DevClientTelescopeImage::getEquTel (struct ln_equ_posn *tel)
{
	tel->ra = getConnection ()->getValueDouble ("MNT_RA");
	tel->dec = getConnection ()->getValueDouble ("MNT_DEC");
}


void
Rts2DevClientTelescopeImage::getEquTar (struct ln_equ_posn *tar)
{
	tar->ra = getConnection ()->getValueDouble ("RASC");
	tar->dec = getConnection ()->getValueDouble ("DECL");
}


int
Rts2DevClientTelescopeImage::getMountFlip ()
{
	return getConnection ()->getValueInteger ("MNT_FLIP");
}


void
Rts2DevClientTelescopeImage::getAltAz (struct ln_hrz_posn *hrz)
{
	struct ln_equ_posn pos;
	struct ln_lnlat_posn obs;
	double gst;

	getEqu (&pos);
	getObs (&obs);
	gst = getLocalSiderealDeg () - obs.lng;
	gst = ln_range_degrees (gst) / 15.0;

	ln_get_hrz_from_equ_sidereal_time (&pos, &obs, gst, hrz);
}


void
Rts2DevClientTelescopeImage::getObs (struct ln_lnlat_posn *obs)
{
	obs->lng = getConnection ()->getValueDouble ("LONGITUD");
	obs->lat = getConnection ()->getValueDouble ("LATITUDE");
}


double
Rts2DevClientTelescopeImage::getLocalSiderealDeg ()
{
	return getConnection ()->getValueDouble ("siderealtime") * 15.0;
}


double
Rts2DevClientTelescopeImage::getDistance (struct ln_equ_posn *in_pos)
{
	struct ln_equ_posn tel;
	getEqu (&tel);
	return ln_get_angular_separation (&tel, in_pos);
}


Rts2DevClientFocusImage::Rts2DevClientFocusImage (Rts2Conn * in_connection):Rts2DevClientFocus
(in_connection)
{
}


void
Rts2DevClientFocusImage::postEvent (Rts2Event * event)
{
	CameraImage * ci;
	Rts2Image *image;
	switch (event->getType ())
	{
		case EVENT_WRITE_TO_IMAGE:
			ci = (CameraImage *) event->getArg ();
			image = ci->image;
			// check if we are correct focuser for given camera
			if (!image->getFocuserName ()
				|| !connection->getName ()
				|| strcmp (image->getFocuserName (), connection->getName ()))
				break;
			image->writeConn (getConnection (), EXPOSURE_START);
			image->setFocPos (getConnection ()->getValue ("FOC_POS")->getValueInteger ());
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
	}
	Rts2DevClientFocus::postEvent (event);
}


Rts2DevClientWriteImage::Rts2DevClientWriteImage (Rts2Conn * in_connection):Rts2DevClient
(in_connection)
{
}


void
Rts2DevClientWriteImage::postEvent (Rts2Event * event)
{
	CameraImage *ci;
	switch (event->getType ())
	{
		case EVENT_WRITE_TO_IMAGE:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_START);
			// and check if we should trigger info call
			if (connection->existWriteType (RTS2_VWHEN_BEFORE_END))
			{
				queCommand (new Rts2CommandInfo (getMaster ()));
				ci->waitForDevice (this, getMaster ()->getNow ());
			}
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
	}
	Rts2DevClient::postEvent (event);
}


void
Rts2DevClientWriteImage::infoOK ()
{
	// see if somebody cares..
	connection->postMaster (new Rts2Event (EVENT_INFO_DEVCLI_OK, this));
}


void
Rts2DevClientWriteImage::infoFailed ()
{
	connection->postMaster (new Rts2Event (EVENT_INFO_DEVCLI_FAILED, this));
}


Rts2CommandQueImage::Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image):Rts2Command
(in_owner)
{
	char *command;
	asprintf (&command, "que_image %s", image->getImageName ());
	setCommand (command);
	free (command);
}


Rts2CommandQueObs::Rts2CommandQueObs (Rts2Block * in_owner, int in_obsId):
Rts2Command (in_owner)
{
	char *command;
	asprintf (&command, "que_obs %i", in_obsId);
	setCommand (command);
	free (command);
}
