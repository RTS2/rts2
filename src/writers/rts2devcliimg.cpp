/* 
 * Client which produces images.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

Rts2DevClientCameraImage::Rts2DevClientCameraImage (Rts2Conn * in_connection):rts2core::Rts2DevClientCamera (in_connection)
{
	chipNumbers = 0;
	saveImage = 1;

	Rts2Config *config = Rts2Config::instance ();

	xoa = 0;
	yoa = 0;
	ter_xoa = nan ("f");
	ter_yoa = nan ("f");

	config->getDouble (connection->getName (), "xoa", xoa);
	config->getDouble (connection->getName (), "yoa", yoa);
	config->getDouble (connection->getName (), "ter_xoa", ter_xoa);
	config->getDouble (connection->getName (), "ter_yoa", ter_yoa);

	telescop[0] = '\0';
	instrume[0] = '\0';
	origin[0] = '\0';

	config->getString (connection->getName (), "instrume", instrume);
	config->getString (connection->getName (), "telescop", telescop);
	config->getString (connection->getName (), "origin", origin);

	actualImage = NULL;
	lastImage = NULL;

	expNum = 0;

	triggered = false;
}

Rts2DevClientCameraImage::~Rts2DevClientCameraImage (void)
{
}

Rts2Image * Rts2DevClientCameraImage::setImage (Rts2Image * old_img, Rts2Image * new_image)
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

void Rts2DevClientCameraImage::postEvent (Rts2Event * event)
{
	bool ret_all,ret_actual;
	switch (event->getType ())
	{
		case EVENT_INFO_DEVCLI_OK:
			images.infoOK (this, (rts2core::Rts2DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((rts2core::Rts2DevClient *) event->getArg ());
			break;
		case EVENT_INFO_DEVCLI_FAILED:
			images.infoFailed (this, (rts2core::Rts2DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((rts2core::Rts2DevClient *) event->getArg ());
			break;
		case EVENT_TRIGGERED:
			ret_all = images.wasTriggered (this, (rts2core::Rts2DevClient *) event->getArg ());
			if (actualImage)
			{
			  	ret_actual = actualImage->wasTriggered ((rts2core::Rts2DevClient *) event->getArg ());
			}
			else
			{
				ret_actual = false;
			}
			// this trigger was not handled..
			if (ret_all == false && ret_actual == false)
			{
				// we have a problem, those data were received before exposure. They are most probably invalid.
				prematurelyReceived.push_back ((rts2core::Rts2DevClient *) event->getArg ());
				std::cout << "will ignore trigger from " << ((rts2core::Rts2DevClient *) event->getArg ())->getName () << std::endl;
			}

			// check that no image is waiting for trigger - e.g. this was last EVENT_TRIGGERED received
			if (actualImage && actualImage->waitForMetaData ())
				break;

			for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
			{
				if (((*iter).second)->waitForMetaData ())
					break;
			}
			// before we check that no image is waiting for trigger - this is last EVENT_TRIGGERED received
			triggered = false;
			break;
		case EVENT_NUMBER_OF_IMAGES:
			*((int *)event->getArg ()) += images.size ();
			if (actualImage)
				*((int *)event->getArg ()) += 1;
			break;
	}
	rts2core::Rts2DevClientCamera::postEvent (event);
}

void Rts2DevClientCameraImage::writeFilter (Rts2Image *img)
{
	int camFilter = img->getFilterNum ();
	char imageFilter[4];
	strncpy (imageFilter, getConnection()->getValueSelection ("filter", camFilter), 4);
	imageFilter[4] = '\0';
	img->setFilter (imageFilter);
}

void Rts2DevClientCameraImage::newDataConn (int data_conn)
{
	if (!actualImage)
	{
		logStream (MESSAGE_ERROR) << "Data stream started without exposure!" << sendLog;
		return;
	}
	images[data_conn] = actualImage;
	actualImage = NULL;
}

void Rts2DevClientCameraImage::fullDataReceived (int data_conn, rts2core::DataChannels *data)
{
	CameraImages::iterator iter = images.find (data_conn);
	if (iter != images.end ())
	{
		CameraImage *ci = (*iter).second;

		if (ci->image->getTargetType (false) == TYPE_TERESTIAL && !isnan (ter_xoa) && !isnan (ter_yoa))
		{
			ci->writeMetaData ((struct imghdr *) ((*(data->begin ()))->getDataBuff ()), ter_xoa, ter_yoa);
		}
		else
		{
			ci->writeMetaData ((struct imghdr *) ((*(data->begin ()))->getDataBuff ()), xoa, yoa);
		}

		for (rts2core::DataChannels::iterator di = data->begin (); di != data->end (); di++)
			ci->writeData ((*di)->getDataBuff (), (*di)->getDataTop (), data->size ());

		ci->image->moveHDU (1);

		cameraImageReady (ci->image);

		if (ci->canDelete ())
		{
			processCameraImage (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "getData, but not all metainfo - size of images:" << images.size () << sendLog;
		}
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "invalid data_conn: " << data_conn << sendLog;
	}
}

Rts2Image * Rts2DevClientCameraImage::createImage (const struct timeval *expStart)
{
	return new Rts2Image ("%c_%y%m%d-%H%M%S-%s.fits", getExposureNumber (), expStart, connection);
}

void Rts2DevClientCameraImage::processCameraImage (CameraImages::iterator cis)
{
	CameraImage *ci = (*cis).second;
	try
	{
		// move to the first HDU before writing data
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
	}
	catch (rts2core::Error &ex)
	{
		logStream (MESSAGE_WARNING) << "Cannot save image " << ci->image->getAbsoluteFileName () << " " << ex << sendLog;
	}

	if (lastImage == ci->image)
		lastImage = NULL;
	delete ci;
	images.erase (cis);
}

void Rts2DevClientCameraImage::beforeProcess (Rts2Image * image)
{
}

imageProceRes Rts2DevClientCameraImage::processImage (Rts2Image * image)
{
	return IMAGE_DO_BASIC_PROCESSING;
}

void Rts2DevClientCameraImage::stateChanged (Rts2ServerState * state)
{
	rts2core::Rts2DevClientCamera::stateChanged (state);
	if (triggered && !(getConnection ()->getFullBopState () & BOP_TRIG_EXPOSE))
		triggered = false;
}

void Rts2DevClientCameraImage::exposureStarted ()
{
	double exposureTime = getConnection ()->getValueDouble ("exposure");
	struct timeval expStart;
	const char *focuser;
	gettimeofday (&expStart, NULL);
	try
	{
		Rts2Image *image = createImage (&expStart);
		if (image == NULL)
			return;
		image->setExposureLength (exposureTime);
	
		image->setCameraName (getName ());
		image->setInstrument (instrume.c_str ());
		image->setTelescope (telescop.c_str ());
		image->setOrigin (origin.c_str ());
	
		image->setEnvironmentalValues ();
	
		focuser = getConnection ()->getValueChar ("focuser");
		if (focuser)
		{
			image->setFocuserName (focuser);
		}
		// delete old image
		delete actualImage;
		// create image
		actualImage = new CameraImage (image, getMaster ()->getNow (), prematurelyReceived);

		prematurelyReceived.clear ();

		actualImage->image->writeConn (getConnection (), EXPOSURE_START);
	
		lastImage = image;
	}
	catch (rts2core::Error &ex)
	{
		logStream (MESSAGE_ERROR) << "cannot create image for exposure " << ex << sendLog;
		return;
	}
	connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE, actualImage));
	rts2core::Rts2DevClientCamera::exposureStarted ();
}

void Rts2DevClientCameraImage::exposureEnd ()
{
	logStream (MESSAGE_DEBUG) << "exposureEnd " << connection->getName () << sendLog;

	if (actualImage)
	{
		actualImage->setExEnd (getMaster ()->getNow ());
		connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE_ENDS, actualImage));
	}

	rts2core::Rts2DevClientCamera::exposureEnd ();
}

bool Rts2DevClientCameraImage::waitForMetaData ()
{
	if (triggered && !(getConnection ()->getFullBopState () & BOP_TRIG_EXPOSE))
		triggered = false;
	if (actualImage && actualImage->waitForMetaData ())
		return true;
	for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		if (((*iter).second)->waitForMetaData ())
			return true;
	}
	// wait if the program is already waiting for an exposure to be send
	if (triggered && (getConnection ()->getFullBopState () & BOP_TRIG_EXPOSE))
		return true;
	return false;
}

Rts2DevClientTelescopeImage::Rts2DevClientTelescopeImage (Rts2Conn * in_connection):rts2core::Rts2DevClientTelescope (in_connection)
{
}

void Rts2DevClientTelescopeImage::postEvent (Rts2Event * event)
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
			image->setValue ("MNT_INFO", infotime, "time when mount informations were collected");

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
			queCommand (new rts2core::Rts2CommandChange (this, change->ra, change->dec));
			break;
	}

	rts2core::Rts2DevClientTelescope::postEvent (event);
}

void Rts2DevClientTelescopeImage::getEqu (struct ln_equ_posn *tel)
{
	Rts2ValueRaDec *vradec = (Rts2ValueRaDec *) getConnection ()->getValue ("TAR");
	if (!vradec)
		return;

	tel->ra = vradec->getRa ();
	tel->dec = vradec->getDec ();
}

void Rts2DevClientTelescopeImage::getEquTel (struct ln_equ_posn *tel)
{
	tel->ra = getConnection ()->getValueDouble ("MNT_RA");
	tel->dec = getConnection ()->getValueDouble ("MNT_DEC");
}

void Rts2DevClientTelescopeImage::getEquTar (struct ln_equ_posn *tar)
{
	tar->ra = getConnection ()->getValueDouble ("RASC");
	tar->dec = getConnection ()->getValueDouble ("DECL");
}

int Rts2DevClientTelescopeImage::getMountFlip ()
{
	return getConnection ()->getValueInteger ("MNT_FLIP");
}

void Rts2DevClientTelescopeImage::getObs (struct ln_lnlat_posn *obs)
{
	obs->lng = getConnection ()->getValueDouble ("LONGITUD");
	obs->lat = getConnection ()->getValueDouble ("LATITUDE");
}

double Rts2DevClientTelescopeImage::getDistance (struct ln_equ_posn *in_pos)
{
	struct ln_equ_posn tel;
	getEqu (&tel);
	return ln_get_angular_separation (&tel, in_pos);
}

Rts2DevClientFocusImage::Rts2DevClientFocusImage (Rts2Conn * in_connection):rts2core::Rts2DevClientFocus (in_connection)
{
}

void Rts2DevClientFocusImage::postEvent (Rts2Event * event)
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
	rts2core::Rts2DevClientFocus::postEvent (event);
}

Rts2DevClientWriteImage::Rts2DevClientWriteImage (Rts2Conn * in_connection):rts2core::Rts2DevClient (in_connection)
{
}

void Rts2DevClientWriteImage::postEvent (Rts2Event * event)
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
				queCommand (new rts2core::Rts2CommandInfo (getMaster ()));
				ci->waitForDevice (this, getMaster ()->getNow ());
			}
			if (connection->existWriteType (RTS2_VWHEN_TRIGGERED))
			{
				ci->waitForTrigger (this);
			}
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
	}
	Rts2DevClient::postEvent (event);
}

void Rts2DevClientWriteImage::infoOK ()
{
	// see if somebody cares..
	connection->postMaster (new Rts2Event (EVENT_INFO_DEVCLI_OK, this));
}

void Rts2DevClientWriteImage::infoFailed ()
{
	connection->postMaster (new Rts2Event (EVENT_INFO_DEVCLI_FAILED, this));
}

void Rts2DevClientWriteImage::stateChanged (Rts2ServerState * state)
{
	Rts2DevClient::stateChanged (state);
	if ((state->getOldValue () & BOP_TRIG_EXPOSE) == 0 && (state->getValue () & BOP_TRIG_EXPOSE) && !(state->getOldValue () & DEVICE_NOT_READY))
		connection->postMaster (new Rts2Event (EVENT_TRIGGERED, this));
}

Rts2CommandQueImage::Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image):rts2core::Rts2Command (in_owner)
{
  	std::ostringstream _os;
	_os << "que_image " << image->getFileName ();
	setCommand (_os);
}

Rts2CommandQueDark::Rts2CommandQueDark (Rts2Block * in_owner, Rts2Image * image):rts2core::Rts2Command (in_owner)
{
  	std::ostringstream _os;
	_os << "que_dark " << image->getFileName ();
	setCommand (_os);
}

Rts2CommandQueFlat::Rts2CommandQueFlat (Rts2Block * in_owner, Rts2Image * image):rts2core::Rts2Command (in_owner)
{
  	std::ostringstream _os;
	_os << "que_flat " << image->getFileName ();
	setCommand (_os);
}

Rts2CommandQueObs::Rts2CommandQueObs (Rts2Block * in_owner, int in_obsId):rts2core::Rts2Command (in_owner)
{
	std::ostringstream _os;
	_os << "que_obs " << in_obsId;
	setCommand (_os);
}
