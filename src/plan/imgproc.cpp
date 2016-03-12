/*
 * Image processor body.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
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

#include "status.h"
#include "rts2script/connimgprocess.h"
#include "rts2script/script.h"

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/devicedb.h"
#else
#include "device.h"
#include "configuration.h"
#endif

namespace rts2plan
{

/**
 * Image processor main class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
#ifdef RTS2_HAVE_PGSQL
class ImageProc:public rts2db::DeviceDb
#else
class ImageProc:public rts2core::Device
#endif
{
	public:
		ImageProc (int argc, char **argv);
		virtual ~ ImageProc (void);

		virtual void postEvent (rts2core::Event * event);
		virtual int idle ();

		virtual int info ();

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual int deleteConnection (rts2core::Connection * conn);

		int que (ConnProcess * newProc);

		int queImage (const char *_path);
		int doImage (const char *_path);

		int queDark (const char *_path);
		int queFlat (const char *_path);

		int queObs (int obsId);

		int queDarks ();
		int queFlats ();

		int checkNotProcessed ();
		void changeRunning (ConnProcess * newImage);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int reloadConfig ();
#ifndef RTS2_HAVE_PGSQL
		virtual int processOption (int opt);
		virtual int init ();
#endif
		virtual void signaledHUP ()
		{
			reloadConfig ();
		}

	private:
#ifndef RTS2_HAVE_PGSQL
		const char *configFile;
#endif
		std::list < ConnProcess * >imagesQue;
		ConnProcess *runningImage;

		rts2core::ValueString *image_glob;

		rts2core::ValueBool *applyCorrections;
		rts2core::ValueInteger *astrometryTimeout;

		rts2core::ValueInteger *goodImages;
		rts2core::ValueInteger *trashImages;
		rts2core::ValueInteger *badImages;

		rts2core::ValueInteger *darkImages;
		rts2core::ValueInteger *flatImages;

		rts2core::ValueString *processedImage;
		rts2core::ValueInteger *queSize;

		rts2core::ValueRaDec *lastRaDec;
		rts2core::ValueRaDec *lastCorrections;

		rts2core::ValueTime *lastGood;
		rts2core::ValueTime *lastTrash;
		rts2core::ValueTime *lastBad;

		rts2core::ValueInteger *nightGoodImages;
		rts2core::ValueInteger *nightTrashImages;
		rts2core::ValueInteger *nightBadImages;

		rts2core::ValueInteger *nightDarks;
		rts2core::ValueInteger *nightFlats;

		int sendStop;			 // if stop running astrometry with stop signal; it ussually doesn't work, so we will use FIFO

		std::string defaultImgProcess;
		std::string defaultObsProcess;
		glob_t imageGlob;
		unsigned int globC;
		int reprocessingPossible;

		const char *last_processed_jpeg;
		const char *last_good_jpeg;
		const char *last_trash_jpeg;
};

};

using namespace rts2plan;

ImageProc::ImageProc (int _argc, char **_argv)
#ifdef RTS2_HAVE_PGSQL
:rts2db::DeviceDb (_argc, _argv, DEVICE_TYPE_IMGPROC, "IMGP")
#else
:rts2core::Device (_argc, _argv, DEVICE_TYPE_IMGPROC, "IMGP")
#endif
{
	runningImage = NULL;

	last_processed_jpeg = last_good_jpeg = last_trash_jpeg = NULL;

	createValue (applyCorrections, "apply_corrections", "apply corrections from astrometry", false, RTS2_VALUE_WRITABLE);
	applyCorrections->setValueBool (true);

	createValue (astrometryTimeout, "astrometry_timeout", "[s] timeout for astrometry processes", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL| RTS2_VALUE_AUTOSAVE);
	astrometryTimeout->setValueInteger (3600);

	createValue (goodImages, "good_astrom", "number of images with astrometry", false);
	goodImages->setValueInteger (0);

	createValue (trashImages, "no_astrom", "number of images without astrometry", false);
	trashImages->setValueInteger (0);

	createValue (badImages, "failed_images", "number of images with failed processing", false);
	badImages->setValueInteger (0);

	createValue (darkImages, "dark_images", "number of darks", false);
	darkImages->setValueInteger (0);
	createValue (flatImages, "flat_images", "number of flats", false);
	flatImages->setValueInteger (0);

	createValue (processedImage, "processed_image", "image being processed at the moment", false);

	createValue (queSize, "queue_size", "number of images waiting for processing", false);
	queSize->setValueInteger (0);

	createValue (lastRaDec, "last_radec", "last correct image coordinates", false);
	createValue (lastCorrections, "last_corrections", "size of last corrections", false, RTS2_DT_DEG_DIST);

	createValue (lastGood, "last_astrom", "last image with correct astrometry", false);
	createValue (lastTrash, "last_noastrom", "last image without astrometry", false);
	createValue (lastBad, "last_failed", "last image with failed processing", false);

	createValue (nightGoodImages, "night_astrom", "number of images with astrometry taken during night", false);
	createValue (nightTrashImages, "night_noastrom", "number of images without astrometry taken during current night", false);
	createValue (nightBadImages, "night_failed", "number of images with failed processing taken during current night", false);

	createValue (nightDarks, "night_darks", "number of dark images taken during night", false);
	createValue (nightFlats, "night_flats", "number of flat images taken during night", false);

	createValue (image_glob, "image_glob", "glob path for images processed in standy mode", false, RTS2_VALUE_WRITABLE);

	imageGlob.gl_pathc = 0;
	imageGlob.gl_offs = 0;
	globC = 0;
	reprocessingPossible = 0;

	sendStop = 0;

#ifndef RTS2_HAVE_PGSQL
	configFile = NULL;
	addOption (OPT_CONFIG, "config", 1, "configuration file");
#endif

#ifdef RTS2_HAVE_LIBJPEG
	Magick::InitializeMagick (".");
#endif
}

ImageProc::~ImageProc (void)
{
	if (imageGlob.gl_pathc)
		globfree (&imageGlob);
}

int ImageProc::reloadConfig ()
{
	int ret;
	
	std::string imgglob;
#ifdef RTS2_HAVE_PGSQL
	ret = rts2db::DeviceDb::reloadConfig ();
#else
	Configuration *config = Configuration::instance ();
	ret = config->loadFile (configFile);
#endif
	if (ret)
		return ret;


	ret = config->getString ("imgproc", "astrometry", defaultImgProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ImageProc::reloadConfig cannot get astrometry string, exiting!" << sendLog;
		return ret;
	}

	ret = config->getString ("imgproc", "obsprocess", defaultObsProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ImageProc::reloadConfig cannot get obs process script, exiting" << sendLog;
	}

	ret = config->getString ("imgproc", "imageglob", imgglob);
	if (ret || imgglob.length () == 0)
		return ret;
	image_glob->setValueCharArr (imgglob.c_str ());

	last_processed_jpeg = config->getStringDefault ("imgproc", "last_processed_jpeg", NULL);
	last_good_jpeg = config->getStringDefault ("imgproc", "last_good_jpeg", NULL);
	last_trash_jpeg = config->getStringDefault ("imgproc", "last_trash_jpeg", NULL);

	astrometryTimeout->setValueInteger (config->getAstrometryTimeout ());

	return ret;
}

#ifndef RTS2_HAVE_PGSQL
int ImageProc::processOption (int opt)
{
	switch (opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		default:
			return rts2core::Device::processOption (opt);
	}
	return 0;
}

int ImageProc::init ()
{
	int ret;
	ret = rts2core::Device::init ();
	if (ret)
		return ret;
	return reloadConfig ();
}
#endif

void ImageProc::postEvent (rts2core::Event * event)
{
	int obsId;
	switch (event->getType ())
	{
		case EVENT_ALL_PROCESSED:
			obsId = *((int *) event->getArg ());
			queObs (obsId);
			break;
	}
#ifdef RTS2_HAVE_PGSQL
	rts2db::DeviceDb::postEvent (event);
#else
	rts2core::Device::postEvent (event);
#endif
}

int ImageProc::idle ()
{
	std::list < ConnProcess * >::iterator img_iter;
	if (!runningImage && imagesQue.size () != 0)
	{
		img_iter = imagesQue.begin ();
		ConnProcess *newImage = *img_iter;
		imagesQue.erase (img_iter);
		changeRunning (newImage);
	}
#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::idle ();
#else
	return rts2core::Device::idle ();
#endif
}

int ImageProc::info ()
{
	queSize->setValueInteger ((int) imagesQue.size () + (runningImage ? 1 : 0));
	sendValueAll (queSize);
#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::info ();
#else
	return rts2core::Device::info ();
#endif
}

void ImageProc::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK))
	{
		case SERVERD_DUSK:
			nightGoodImages->setValueInteger (0);
			nightTrashImages->setValueInteger (0);
			nightBadImages->setValueInteger (0);
			nightDarks->setValueInteger (0);
			nightFlats->setValueInteger (0);
			
			sendValueAll (nightGoodImages);
			sendValueAll (nightTrashImages);
			sendValueAll (nightBadImages);
			sendValueAll (nightDarks);
			sendValueAll (nightFlats);
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			if (!(new_state & SERVERD_ONOFF_MASK))
			{
				if (imageGlob.gl_pathc)
				{
					globfree (&imageGlob);
					imageGlob.gl_pathc = 0;
					globC = 0;
				}
				reprocessingPossible = 0;
				break;
			}
		default:
			if (strlen (image_glob->getValue ()))
			{
				reprocessingPossible = 1;
				if (!runningImage && imagesQue.size () == 0)
					checkNotProcessed ();
			}
	}
	// start dark & flat processing
#ifdef RTS2_HAVE_PGSQL
	rts2db::DeviceDb::changeMasterState (old_state, new_state);
#else
	rts2core::Device::changeMasterState (old_state, new_state);
#endif
}

int ImageProc::deleteConnection (rts2core::Connection * conn)
{
	std::list < ConnProcess * >::iterator img_iter;
	for (img_iter = imagesQue.begin (); img_iter != imagesQue.end ();)
	{
		(*img_iter)->deleteConnection (conn);
		if (*img_iter == conn)
		{
			img_iter = imagesQue.erase (img_iter);
		}
		else
		{
			img_iter++;
		}
	}
	queSize->setValueInteger (imagesQue.size ());
	sendValueAll (queSize);
	if (runningImage)
		runningImage->deleteConnection (conn);
	if (conn == runningImage)
	{
		// que next image
		// rts2core::Device::deleteConnection will delete runningImage
		switch (runningImage->getAstrometryStat ())
		{
			case NOT_ASTROMETRY:
				break;	
			case GET:
				goodImages->inc ();
				nightGoodImages->inc ();
				lastRaDec->setValueRaDec (((ConnImgOnlyProcess *) runningImage)->getRa (), ((ConnImgOnlyProcess *) runningImage)->getDec ());
				lastCorrections->setValueRaDec (((ConnImgOnlyProcess *) runningImage)->getRaErr (), ((ConnImgOnlyProcess *) runningImage)->getDecErr ());
				sendValueAll (goodImages);
				sendValueAll (nightGoodImages);
				sendValueAll (lastRaDec);
				sendValueAll (lastCorrections);
				if (isnan (lastGood->getValueDouble ()) || runningImage->getExposureEnd () > lastGood->getValueDouble ())
				{
					lastGood->setValueDouble (runningImage->getExposureEnd ());
					sendValueAll (lastGood);
				}
				break;
			case TRASH:
				trashImages->inc ();
				nightTrashImages->inc ();
				sendValueAll (trashImages);
				sendValueAll (nightTrashImages);
				if (isnan (lastTrash->getValueDouble ()) || runningImage->getExposureEnd () > lastTrash->getValueDouble ())
				{
					lastTrash->setValueDouble (runningImage->getExposureEnd ());
					sendValueAll (lastTrash);
				}
				break;
			case BAD:
				badImages->inc ();
				nightBadImages->inc ();
				sendValueAll (badImages);
				sendValueAll (nightBadImages);
				lastBad->setValueDouble (getNow ());
				sendValueAll (lastBad);
				break;
			case FLAT:
				flatImages->inc ();
				nightFlats->inc ();
				sendValueAll (flatImages);
				sendValueAll (nightFlats);
				break;
			case DARK:
				darkImages->inc ();
				nightDarks->inc ();
				sendValueAll (darkImages);
				sendValueAll (nightDarks);
				break;
			default:
				logStream (MESSAGE_ERROR) << "wrong image state: " << runningImage->getAstrometryStat () << sendLog;
				break;
		}
		runningImage = NULL;
		img_iter = imagesQue.begin ();
		if (img_iter != imagesQue.end ())
		{
			ConnProcess *cp = *img_iter;
			imagesQue.erase (img_iter);
			changeRunning (cp);
		}
		// still not image process running..
		if (runningImage == NULL)
		{
			maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_IDLE);
			if (reprocessingPossible)
			{
				if (imageGlob.gl_pathc > 0)
				{
					globC++;
					if (globC < imageGlob.gl_pathc)
					{
						queImage (imageGlob.gl_pathv[globC]);
					}
					else
					{
						globfree (&imageGlob);
						imageGlob.gl_pathc = 0;
					}
				}
			}
		}
	}
#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::deleteConnection (conn);
#else
	return rts2core::Device::deleteConnection (conn);
#endif
}

void ImageProc::changeRunning (ConnProcess * newImage)
{
	int ret;
	if (runningImage)
	{
		if (sendStop)
		{
			runningImage->stop ();
			imagesQue.push_front (runningImage);
		}
		else
		{
			imagesQue.push_front (newImage);
			infoAll ();
			return;
		}
	}
	runningImage = newImage;
        runningImage->setConnectionDebug (getDebug ());
	ret = runningImage->init ();
	if (ret < 0)
	{
		deleteConnection (runningImage);
		delete runningImage;
		runningImage = NULL;
		maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, DEVICE_ERROR_HW | IMGPROC_IDLE);
		infoAll ();
		if (reprocessingPossible)
			checkNotProcessed ();
		return;
	}
	else if (ret == 0)
	{
#ifdef RTS2_HAVE_LIBJPEG
		if (isnan (lastGood->getValueDouble ()) || lastGood->getValueDouble () < runningImage->getExposureEnd ())
			runningImage->setLastGoodJpeg (last_good_jpeg);
		if (isnan (lastGood->getValueDouble ()) || lastTrash->getValueDouble() < runningImage->getExposureEnd ())
			runningImage->setLastTrashJpeg (last_trash_jpeg);
#endif
		addConnection (runningImage);
		processedImage->setValueCharArr (runningImage->getProcessArguments ());
	}
	maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_RUN);
	infoAll ();
}

int ImageProc::que (ConnProcess * newProc)
{
	if (runningImage)
		imagesQue.push_front (newProc);
	else
		changeRunning (newProc);
	infoAll ();
	return 0;
}

int ImageProc::queImage (const char *_path)
{
	ConnImgProcess *newImageConn;
	newImageConn = new ConnImgProcess (this, defaultImgProcess.c_str (), _path, astrometryTimeout->getValueInteger ());
	return que (newImageConn);
}

int ImageProc::doImage (const char *_path)
{
	ConnImgProcess *newImageConn;
	newImageConn = new ConnImgProcess (this, defaultImgProcess.c_str (), _path, astrometryTimeout->getValueInteger ());
	changeRunning (newImageConn);
	infoAll ();
	return 0;
}

int ImageProc::queObs (int obsId)
{
	ConnObsProcess *newObsConn;
	newObsConn = new ConnObsProcess (this, defaultObsProcess.c_str (), obsId, astrometryTimeout->getValueInteger ());
	return que (newObsConn);
}

int ImageProc::checkNotProcessed ()
{
	int ret;

	ret = glob (image_glob->getValue (), 0, NULL, &imageGlob);
	if (ret)
	{
		globfree (&imageGlob);
		imageGlob.gl_pathc = 0;
		return -1;
	}

	globC = 0;

	// start files que..
	if (imageGlob.gl_pathc > 0)
		return queImage (imageGlob.gl_pathv[0]);
	return 0;
}

int ImageProc::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("que_image"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName) || !conn->paramEnd ())
			return -2;
		return queImage (in_imageName);
	}
	else if (conn->isCommand ("only_process"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName))
			return -2;
		ConnProcess *newConn;
		newConn = new ConnImgOnlyProcess (this, defaultImgProcess.c_str (), in_imageName, astrometryTimeout->getValueInteger ());

		while (!conn->paramNextString (&in_imageName))
			newConn->addArg (in_imageName);

		return que (newConn);
	}
	else if (conn->isCommand ("do_image"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName) || !conn->paramEnd ())
			return -2;
		return doImage (in_imageName);
	}
	else if (conn->isCommand ("que_obs"))
	{
		int obsId;
		if (conn->paramNextInteger (&obsId) || !conn->paramEnd ())
			return -2;
		return queObs (obsId);
	}
#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::commandAuthorized (conn);
#else
	return rts2core::Device::commandAuthorized (conn);
#endif
}

int main (int argc, char **argv)
{
	ImageProc imgproc = ImageProc (argc, argv);
	return imgproc.run ();
}
