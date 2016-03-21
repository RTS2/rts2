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

#include "rts2fits/devcliimg.h"
#include "iniparser.h"
#include "configuration.h"
#include "valuerectangle.h"
#include "timestamp.h"

using namespace rts2image;

DevClientCameraImage::DevClientCameraImage (rts2core::Connection * in_connection, std::string templateFile):rts2core::DevClientCamera (in_connection)
{
	chipNumbers = 0;
	saveImage = 1;

	fitsTemplate = NULL;

	rts2core::Configuration *config = rts2core::Configuration::instance ();

	telescop[0] = '\0';
	instrume[0] = '\0';
	origin[0] = '\0';

	config->getString (connection->getName (), "instrume", instrume);
	config->getString (connection->getName (), "telescop", telescop);
	config->getString (connection->getName (), "origin", origin);

	writeConnection = true;
	writeRTS2Values = true;

	// load template file..
	if (templateFile.length () == 0)
	{
		config->getString (connection->getName (), "template", templateFile);
		if (templateFile.length () > 0)
		{
			writeConnection = !(config->getBoolean (connection->getName (), "no-metadata", true));
			writeRTS2Values = writeConnection;
		}
	}

	if (templateFile.length () > 0)
	{
		fitsTemplate = new rts2core::IniParser ();
		if (fitsTemplate->loadFile (templateFile.c_str (), true))
		{
			delete fitsTemplate;
			fitsTemplate = NULL;
			logStream (MESSAGE_ERROR) << "cannot load FITS template from " << templateFile << sendLog;
		}
		else
		{
			logStream (MESSAGE_DEBUG) << "loaded FITS template from " << templateFile<< sendLog;
		}
	}

	actualImage = NULL;

	expNum = 0;

	triggered = false;
}

DevClientCameraImage::~DevClientCameraImage (void)
{
	delete fitsTemplate;
	delete actualImage;
}

Image * DevClientCameraImage::setImage (Image * old_img, Image * new_image)
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

void DevClientCameraImage::postEvent (rts2core::Event * event)
{
	bool ret_all,ret_actual;
	switch (event->getType ())
	{
		case EVENT_INFO_DEVCLI_OK:
			images.infoOK (this, (rts2core::DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((rts2core::DevClient *) event->getArg ());
			break;
		case EVENT_INFO_DEVCLI_FAILED:
			images.infoFailed (this, (rts2core::DevClient *) event->getArg ());
			// check also actualImage
			if (actualImage)
				actualImage->waitingFor ((rts2core::DevClient *) event->getArg ());
			break;
		case EVENT_TRIGGERED:
			ret_all = images.wasTriggered (this, (rts2core::DevClient *) event->getArg ());
			if (actualImage)
			{
			  	ret_actual = actualImage->wasTriggered ((rts2core::DevClient *) event->getArg ());
			}
			else
			{
				ret_actual = false;
			}
			// this trigger was not handled..
			if (ret_all == false && ret_actual == false)
			{
				// we have a problem, those data were received before exposure. They are most probably invalid.
				prematurelyReceived.push_back ((rts2core::DevClient *) event->getArg ());
				std::cout << "will ignore trigger from " << ((rts2core::DevClient *) event->getArg ())->getName () << std::endl;
			}

			// check that no image is waiting for trigger - e.g. this was last EVENT_TRIGGERED received
			if (actualImage && actualImage->waitForMetaData ())
				break;

			for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
			{
				if (iter->second->waitForMetaData ())
					break;
			}
			// before we check that no image is waiting for trigger - this is last EVENT_TRIGGERED received
			triggered = false;
			break;
		case EVENT_KILL_ALL:
			for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
			{
				delete iter->second;
			}
			images.clear ();
			delete actualImage;
			actualImage = NULL;
			break;
		case EVENT_NUMBER_OF_IMAGES:
			*((int *)event->getArg ()) += images.size ();
			if (actualImage)
				*((int *)event->getArg ()) += 1;
			break;
		case EVENT_METADATA_TIMEOUT:
			if (checkImages.size ())
			{
				for (CameraImages::iterator iter = images.begin (); iter != images.end (); iter++)
				{
					if (iter->second->image != checkImages[0])
						continue;
					processCameraImage (iter);
					break;
				}
			}
			break;
	}
	rts2core::DevClientCamera::postEvent (event);
}

void DevClientCameraImage::cameraMetadata (Image *image)
{
	double exposureTime = getConnection ()->getValueDouble ("exposure");
	image->setTemplate (fitsTemplate);

	image->setExposureLength (exposureTime);
	
	image->setCameraName (getName ());
	image->setInstrument (instrume.c_str ());
	image->setTelescope (telescop.c_str ());
	image->setOrigin (origin.c_str ());

	rts2core::Value *wcsaux = getConnection ()->getValue ("WCSAUX");
	if (wcsaux && wcsaux->getValueBaseType () == RTS2_VALUE_STRING && wcsaux->getValueExtType () == RTS2_VALUE_ARRAY)
		image->setAUXWCS ((rts2core::StringArray *) wcsaux);

	image->setEnvironmentalValues ();

	const char *focuser = getConnection ()->getValueChar ("focuser");
	if (focuser)
	{
		image->setFocuserName (focuser);
	}
}

void DevClientCameraImage::fits2DataChannels (Image *img, rts2core::DataChannels *&data)
{
	data = new rts2core::DataChannels ();
	// add channels one by one
	for (int i = 0; i < img->getChannelSize (); i++)
	{
		struct imghdr imgh;
		img->getImgHeader (&imgh, i);
		long ts = img->getChannelNPixels (i) * img->getPixelByteSize ();
		rts2core::DataRead *dr = new rts2core::DataRead (ts + sizeof (struct imghdr), img->getDataType ());
		dr->setChunkSizeFromData ();
		dr->addData ((char *) (&imgh), sizeof (struct imghdr));
		dr->addData ((char *) img->getChannelData (i), ts);
		data->push_back (dr);
	}
}

void DevClientCameraImage::writeFilter (Image *img)
{
	int camFilter = img->getFilterNum ();
	char imageFilter[5];
	strncpy (imageFilter, getConnection()->getValueSelection ("filter", camFilter), 4);
	imageFilter[4] = '\0';
	img->setFilter (imageFilter);
}

void DevClientCameraImage::newDataConn (int data_conn)
{
	if (!actualImage)
	{
		logStream (MESSAGE_ERROR) << "Data stream of camera " << getName () << " started without exposure!" << sendLog;
		return;
	}
	images[data_conn] = actualImage;
	actualImage = NULL;
}

void DevClientCameraImage::allImageDataReceived (int data_conn, rts2core::DataChannels *data, bool data2fits)
{
	CameraImages::iterator iter = images.find (data_conn);
	if (data_conn == -1 && iter == images.end ())
	{
		newDataConn (data_conn);
		iter = images.find (data_conn);
	}

	images.deleteOld ();

	if (iter != images.end ())
	{
		CameraImage *ci = (*iter).second;

		ci->writeMetaData ((struct imghdr *) ((*(data->begin ()))->getDataBuff ()));

		// detector coordinates,..
		rts2core::ValueRectangle *detsize = getRectangle ("DETSIZE");

		rts2core::DoubleArray *chan1_offsets = getDoubleArray ("CHAN1_OFFSETS");
		rts2core::DoubleArray *chan2_offsets = getDoubleArray ("CHAN2_OFFSETS");

		rts2core::DoubleArray *chan1_delta = getDoubleArray ("CHAN1_DELTA");
		rts2core::DoubleArray *chan2_delta = getDoubleArray ("CHAN2_DELTA");

		rts2core::DoubleArray *trim_x = getDoubleArray ("TRIM_X1");
		rts2core::DoubleArray *trim_y = getDoubleArray ("TRIM_Y1");
		rts2core::DoubleArray *trim_x2 = getDoubleArray ("TRIM_X2");
		rts2core::DoubleArray *trim_y2 = getDoubleArray ("TRIM_Y2");

		for (rts2core::DataChannels::iterator di = data->begin (); di != data->end (); di++)
		{
			ci->writeData ((*di)->getDataBuff (), (*di)->getDataTop (), data2fits ? data->size () : -data->size ());

			struct imghdr *imgh = (struct imghdr *) ((*di)->getDataBuff ());

			uint16_t chan = ntohs (imgh->channel) - 1;

			int16_t x = ntohs (imgh->x);
			int16_t y = ntohs (imgh->y);
			int32_t w = ntohl (imgh->sizes[0]);
			int32_t h = ntohl (imgh->sizes[1]);
			int16_t bin1 = ntohs (imgh->binnings[0]);
			int16_t bin2 = ntohs (imgh->binnings[1]);

			// TV, TM - vector, matrixes
			// for the momemt we assume detector == physical
			double mods[NUM_WCS_VALUES] = {0, 0, 0, 0, 1, 1, 0};

			if (chan1_offsets && chan < chan1_offsets->size ())
				mods[2] += (*chan1_offsets)[chan];
			if (chan2_offsets && chan < chan2_offsets->size ())
				mods[3] += (*chan2_offsets)[chan];
			if (chan1_delta && chan < chan1_delta->size ())
				mods[4] *= (*chan1_delta)[chan];
			if (chan2_delta && chan < chan2_delta->size ())
				mods[5] *= (*chan2_delta)[chan];

			// not sure about this, mayby should be commented out, same as following rows
			// please change if you use channels features and will encounter problems
			if (mods[4] > 0)
				mods[2] *= -1;
			if (mods[5] > 0)
				mods[3] *= -1;

			if (bin1 != 0)
			{
				mods[2] /= bin1;
			}

			if (bin2 != 0)
			{
				mods[3] /= bin2;
			}

			ci->image->writeWCS (mods);

			if (detsize)
			{
				// write detector/channel orientation
				ci->image->setValueRectange ("DETSIZE", detsize->getX ()->getValueDouble (), detsize->getWidth ()->getValueDouble (), detsize->getY ()->getValueDouble (), detsize->getHeight ()->getValueDouble (), "unbined detector size");
				ci->image->setValueRectange ("DATASEC", 1, w, 1, h, "data binned section");

				if (chan1_delta && chan < chan1_delta->size () && chan2_delta && chan < chan2_delta->size () && chan1_offsets && chan < chan1_offsets->size () && chan2_offsets && chan < chan2_offsets->size ())
				{
					double xx = (*chan1_offsets)[chan] + ((*chan1_delta)[chan] > 0 ? 1 : -1) * x;
					double yy = (*chan2_offsets)[chan] + ((*chan2_delta)[chan] > 0 ? 1 : -1) * y;
					ci->image->setValueRectange ("DETSEC",
						xx,
						xx + (*chan1_delta)[chan] * w * bin1,
						yy,
						yy + (*chan2_delta)[chan] * h * bin2,
						"unbinned section of detector");
					// write trim
					if (trim_x || trim_y || trim_x2 || trim_y2)
					{
						double tx = NAN;
						double ty = NAN;
						double tx2 = NAN;
						double ty2 = NAN;

						if (trim_x && chan < trim_x->size ())
							tx = (*trim_x)[chan] - x;
					 	if (trim_y && chan < trim_y->size ())
							ty = (*trim_y)[chan] - y;
						if (trim_x2 && chan < trim_x2->size ())
							tx2 = (*trim_x2)[chan] - x;
						if (trim_y2 && chan < trim_y2->size ())
							ty2 = (*trim_y2)[chan] - y;

						if (tx <= 0)
							tx = 1;
						if (ty <= 0)
							ty = 1;
						if (tx2 <= 0)
							tx2 = 1;
						if (ty2 <= 0)
							ty2 = 1;

						// bin X and Y
						tx /= bin1;
						ty /= bin2;

						tx2 /= bin1;
						ty2 /= bin2;

						if (tx2 > w)
							tx2 = w;
						if (ty2 > h)
							ty2 = h;

						if (tx > tx2)
						{
							tx = 1;
							tx2 = 0;
						}
						if (ty > ty2)
						{
							ty = 1;
							ty2 = 0;
						}
						ci->image->setValueRectange ("TRIMSEC", tx, tx2, ty, ty2, "TRIM binned section");
					}
				}

				std::ostringstream ccdsum;
				ccdsum << bin1 << " " << bin2;
				ci->image->setValue ("CCDSUM", ccdsum.str ().c_str (), "CCD binning");

				ci->image->setValue ("LTV1", mods[2], "image beginning - detector X coordinate");
				ci->image->setValue ("LTV2", mods[3], "image beginning - detector Y coordinate");
				ci->image->setValue ("LTM1_1", mods[4], "delta along X axis");
				ci->image->setValue ("LTM2_2", mods[5], "delta along Y axis");

				ci->image->setValue ("DTV1", 0, "detector transformation vector");
				ci->image->setValue ("DTV2", 0, "detector transformation vector");
				ci->image->setValue ("DTM1_1", 1, "detector transformation matrix");
				ci->image->setValue ("DTM2_2", 1, "detector transformation matrix");
			}
		}

		ci->image->moveHDU (1);

		cameraImageReady (ci->image);

		if (ci->canDelete ())
		{
			processCameraImage (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "getData, but not all metainfo - size of images:" << images.size () << sendLog;
			getMaster ()->addTimer (180, new rts2core::Event (EVENT_METADATA_TIMEOUT, this));
			checkImages.push_back (iter->second->image);
		}
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "invalid data_conn: " << data_conn << sendLog;
	}
}

void DevClientCameraImage::fitsData (const char *fn)
{
	Image *img = new Image ();
	rts2core::DataChannels *data = NULL;

	try
	{
		img->openFile (fn, true, false);

		if (actualImage == NULL)
		{
			logStream (MESSAGE_ERROR) << "FITS data received without exposure start, file " << fn << " kept at its location" << sendLog;
			return;
		}
		std::string abs = std::string (actualImage->image->getAbsoluteFileName ());
		actualImage->image->deleteImage ();
		delete actualImage;
		actualImage = NULL;

		img->copyImage (abs.c_str ());
		img->closeFile ();
		img->openFile (abs.c_str (), false, false);
		img->loadChannels ();
		// convert FITS to data
		fits2DataChannels (img, data);

		img->unkeepImage ();

		images[0] = new CameraImage (img, getNow (), prematurelyReceived);
		images[0]->setExEnd (getNow ());
		writeToFitsTransfer (img);

		allImageDataReceived (0, data, false);
	}
	catch (rts2core::Error &ex)
	{
		logStream (MESSAGE_ERROR) << "cannot process fits_data image " << img->getAbsoluteFileName () << " " << ex << sendLog;
		exposureFailed (-1);
	}
	delete data;

	delete actualImage;
	actualImage = NULL;

	images.erase (0);
}

Image * DevClientCameraImage::createImage (const struct timeval *expStart)
{
	Image * ret = new Image ("%c_%y%m%d-%H%M%S-%s.fits", getExposureNumber (), expStart, connection, false, writeConnection, writeRTS2Values);
	return ret;
}

void DevClientCameraImage::processCameraImage (CameraImages::iterator cis)
{
	CameraImage *ci = (*cis).second;
	std::vector <Image *>::iterator chi = std::find (checkImages.begin (), checkImages.end (), ci->image);
	if (chi != checkImages.end ())
	{
		checkImages.erase (chi);
	}
	try
	{
		writeFilter (ci->image);
		// move to the first HDU before writing data
		beforeProcess (ci->image);
		if (saveImage)
		{
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
	}
	catch (rts2core::Error &ex)
	{
		logStream (MESSAGE_WARNING) << "Cannot save image " << ci->image->getAbsoluteFileName () << " " << ex << sendLog;
	}

	delete ci;
	images.erase (cis);
	// send event that there aren't any images waiting to be written
	if (images.size () == 0 && actualImage == NULL)
		getMaster ()->postEvent (new rts2core::Event (EVENT_ALL_IMAGES_WRITTEN));
}

void DevClientCameraImage::beforeProcess (Image * image)
{
}

imageProceRes DevClientCameraImage::processImage (Image * image)
{
	return IMAGE_DO_BASIC_PROCESSING;
}

void DevClientCameraImage::stateChanged (rts2core::ServerState * state)
{
	rts2core::DevClientCamera::stateChanged (state);
	if (triggered && !(getConnection ()->getFullBopState () & BOP_TRIG_EXPOSE))
		triggered = false;
}

void DevClientCameraImage::exposureStarted (bool expectImage)
{
	struct timeval expStart;
	gettimeofday (&expStart, NULL);

	Image *image = NULL;

	// don't create image if it is not expected
	if (expectImage == false)
		rts2core::DevClientCamera::exposureStarted (expectImage);

	try
	{
		image = createImage (&expStart);

		if (image == NULL)
			return;
		cameraMetadata (image);

		const char *last_filename = image->getAbsoluteFileName ();
		if (last_filename)
			queCommand (new rts2core::CommandChangeValue (this, "last_image", '=', std::string (last_filename)));

		// delete old image
		delete actualImage;
		// create image
		actualImage = new CameraImage (image, getNow (), prematurelyReceived);

		prematurelyReceived.clear ();
		
		actualImage->image->writePrimaryHeader (getName ());
		actualImage->image->writeConn (getConnection (), EXPOSURE_START);
	}
	catch (rts2core::Error &ex)
	{
		if (image)
			logStream (MESSAGE_ERROR) << "cannot create image " << image->getAbsoluteFileName () << " for exposure " << ex << sendLog;
		else
			logStream (MESSAGE_ERROR) << "error creating image:" << ex << sendLog;
		return;
	}
	connection->postMaster (new rts2core::Event (EVENT_WRITE_TO_IMAGE, actualImage));
	rts2core::DevClientCamera::exposureStarted (expectImage);
}

void DevClientCameraImage::exposureEnd (bool expectImage)
{
	logStream (MESSAGE_DEBUG) << "end of camera " << connection->getName () << " exposure" << sendLog;

	if (expectImage == true && actualImage)
	{
		actualImage->setExEnd (getNow ());
		connection->postMaster (new rts2core::Event (EVENT_WRITE_TO_IMAGE_ENDS, actualImage));
	}

	rts2core::DevClientCamera::exposureEnd (expectImage);
}

bool DevClientCameraImage::waitForMetaData ()
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

void DevClientCameraImage::writeToFitsTransfer (Image *img)
{
	img->setCameraName (getConnection ()->getName ());
	img->setValue ("CCD_NAME", getConnection ()->getName (), "camera name");
	connection->postMaster (new rts2core::Event (EVENT_WRITE_TO_IMAGE, images[0]));
	img->writeExposureStart ();

	cameraMetadata (img);

	prematurelyReceived.clear ();
		
	img->writePrimaryHeader (getName ());

	img->writeConn (getConnection (), EXPOSURE_START);
	img->writeConn (getConnection (), EXPOSURE_END);
}

rts2core::DoubleArray * DevClientCameraImage::getDoubleArray (const char *name)
{
	rts2core::Value *v = getConnection ()->getValue (name);
	if (v == NULL || !(v->getValueExtType () == RTS2_VALUE_ARRAY && v->getValueBaseType () == RTS2_VALUE_DOUBLE))
		return NULL;
	return (rts2core::DoubleArray *) v;
}

rts2core::ValueRectangle * DevClientCameraImage::getRectangle (const char *name)
{
	rts2core::Value *v = getConnection ()->getValue (name);
	if (v == NULL || !(v->getValueExtType () == RTS2_VALUE_RECTANGLE))
		return NULL;
	return (rts2core::ValueRectangle *) v;
}

DevClientTelescopeImage::DevClientTelescopeImage (rts2core::Connection * in_connection):rts2core::DevClientTelescope (in_connection)
{
}

void DevClientTelescopeImage::postEvent (rts2core::Event * event)
{
	struct ln_equ_posn *change;	 // change in degrees
	CameraImage * ci;
	Image *image;
	switch (event->getType ())
	{
		case EVENT_WRITE_TO_IMAGE:
			struct ln_equ_posn object;
			struct ln_lnlat_posn obs;
			struct ln_equ_posn suneq;
			struct ln_hrz_posn sunhrz;
			double infotime;
			ci = (CameraImage *) event->getArg ();
			image = ci->image;
			image->setMountName (connection->getName ());
			image->writeConn (getConnection (), EXPOSURE_START);

			getEqu (&object);
			getObs (&obs);

			infotime = getConnection ()->getValueDouble ("infotime");
			image->setRTS2Value ("MNT_INFO", infotime, "time when mount informations were collected");

			ln_get_solar_equ_coords (image->getExposureJD (), &suneq);
			ln_get_hrz_from_equ (&suneq, &obs, image->getExposureJD (), &sunhrz);
			image->setRTS2Value ("SUN_ALT", sunhrz.alt, "solar altitude");
			image->setRTS2Value ("SUN_AZ", sunhrz.az, "solar azimuth");
			break;
		case EVENT_WRITE_ONLY_IMAGE:
			image = (Image *) event->getArg ();
			image->writeConn (getConnection (), EXPOSURE_START);
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
			queCommand (new rts2core::CommandChange (this, change->ra, change->dec));
			break;
	}

	rts2core::DevClientTelescope::postEvent (event);
}

void DevClientTelescopeImage::getEqu (struct ln_equ_posn *tel)
{
	rts2core::ValueRaDec *vradec = (rts2core::ValueRaDec *) getConnection ()->getValue ("TAR");
	if (!vradec)
		return;

	tel->ra = vradec->getRa ();
	tel->dec = vradec->getDec ();
}

void DevClientTelescopeImage::getObs (struct ln_lnlat_posn *obs)
{
	obs->lng = getConnection ()->getValueDouble ("LONGITUD");
	obs->lat = getConnection ()->getValueDouble ("LATITUDE");
}

double DevClientTelescopeImage::getDistance (struct ln_equ_posn *in_pos)
{
	struct ln_equ_posn tel;
	getEqu (&tel);
	return ln_get_angular_separation (&tel, in_pos);
}

DevClientFocusImage::DevClientFocusImage (rts2core::Connection * in_connection):rts2core::DevClientFocus (in_connection)
{
}

void DevClientFocusImage::postEvent (rts2core::Event * event)
{
	CameraImage * ci;
	Image *image;
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
		case EVENT_WRITE_ONLY_IMAGE:
			image = (Image *) event->getArg ();
			image->writeConn (getConnection (), EXPOSURE_START);
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
	}
	rts2core::DevClientFocus::postEvent (event);
}

DevClientWriteImage::DevClientWriteImage (rts2core::Connection * in_connection):rts2core::DevClient (in_connection)
{
}

void DevClientWriteImage::postEvent (rts2core::Event * event)
{
	CameraImage *ci;
	Image *image;
	switch (event->getType ())
	{
		case EVENT_WRITE_TO_IMAGE:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_START);
			// and check if we should trigger info call
			if (connection->existWriteType (RTS2_VWHEN_BEFORE_END))
			{
				queCommand (new rts2core::CommandInfo (getMaster ()));
				ci->waitForDevice (this, getNow ());
			}
			if (connection->existWriteType (RTS2_VWHEN_TRIGGERED))
			{
				ci->waitForTrigger (this);
			}
			break;
		case EVENT_WRITE_ONLY_IMAGE:
			image = (Image *) event->getArg ();
			image->writeConn (getConnection (), EXPOSURE_START);
			break;
		case EVENT_WRITE_TO_IMAGE_ENDS:
			ci = (CameraImage *) event->getArg ();
			ci->image->writeConn (getConnection (), EXPOSURE_END);
			break;
	}
	rts2core::DevClient::postEvent (event);
}

void DevClientWriteImage::infoOK ()
{
	// see if somebody cares..
	connection->postMaster (new rts2core::Event (EVENT_INFO_DEVCLI_OK, this));
}

void DevClientWriteImage::infoFailed ()
{
	connection->postMaster (new rts2core::Event (EVENT_INFO_DEVCLI_FAILED, this));
}

void DevClientWriteImage::stateChanged (rts2core::ServerState * state)
{
	rts2core::DevClient::stateChanged (state);
	if ((state->getOldValue () & BOP_TRIG_EXPOSE) == 0 && (state->getValue () & BOP_TRIG_EXPOSE) && !(state->getOldValue () & DEVICE_NOT_READY))
		connection->postMaster (new rts2core::Event (EVENT_TRIGGERED, this));
}

CommandQueImage::CommandQueImage (rts2core::Block * in_owner, Image * image):rts2core::Command (in_owner)
{
  	std::ostringstream _os;
	_os << "que_image " << image->getFileName ();
	setCommand (_os);
}

CommandQueObs::CommandQueObs (rts2core::Block * in_owner, int in_obsId):rts2core::Command (in_owner)
{
	std::ostringstream _os;
	_os << "que_obs " << in_obsId;
	setCommand (_os);
}
