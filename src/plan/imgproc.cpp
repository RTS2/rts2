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
#include <sys/statvfs.h>
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

		int getFreeSlot ();

		int que (ConnProcess * newProc);

		int queImage (const char *_path);
		int doImage (const char *_path);

		int queDark (const char *_path);
		int queFlat (const char *_path);

		int queObs (int obsId);

		int queDarks ();
		int queFlats ();

		int queNextFromGlob ();
		int numRunning ();

		int checkNotProcessed ();
		void changeRunning (ConnProcess * newImage, int slot);

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
		ConnProcess **runningImage;

		rts2core::ValueString *image_glob;

		rts2core::ValueBool *applyCorrections;
		rts2core::ValueInteger *astrometryTimeout;

		rts2core::ValueInteger *goodImages;
		rts2core::ValueInteger *trashImages;
		rts2core::ValueInteger *badImages;

		rts2core::ValueInteger *darkImages;
		rts2core::ValueInteger *flatImages;
		rts2core::ValueInteger *numObs;

		rts2core::ValueString *processedImage;
		rts2core::ValueInteger *queSize;
		rts2core::ValueInteger *numProc;

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

		rts2core::ValueDouble *freeDiskSpace;

		int sendStop;			 // if stop running astrometry with stop signal; it ussually doesn't work, so we will use FIFO

		std::string defaultImgProcess;
		std::string defaultObsProcess;
		glob_t imageGlob;
		unsigned int globPos;
		int reprocessingPossible;

		std::string last_processed_jpeg;
		std::string last_good_jpeg;
		std::string last_trash_jpeg;
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
	createValue (numObs, "num_observations", "number of observations", false);
	numObs->setValueInteger (0);

	createValue (processedImage, "processed_image", "image being processed at the moment", false);

	createValue (queSize, "queue_size", "number of images waiting for processing", false);
	queSize->setValueInteger (0);

	createValue (numProc, "num_proc", "maximum number of simultaneously running image processing job", false);

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

	createValue (freeDiskSpace, "free_diskspace", "free disk space on the device where we store files, in bytes", false);
	freeDiskSpace->setValueDouble (0);

	imageGlob.gl_pathc = 0;
	imageGlob.gl_offs = 0;
	globPos = 0;
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
	if (runningImage)
		delete[] runningImage;
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

	int np = config->getIntegerDefault ("imgproc", "num_proc", 1);
	numProc->setValueInteger (np);
	if (runningImage == NULL)
		runningImage = new ConnProcess*[np];
	if (runningImage == NULL)
	{
		logStream (MESSAGE_ERROR) << "ImageProc::reloadConfig cannot allocate memory, exiting" << sendLog;
		return -1;
	}
	for (int i = 0; i < np; i++)
		runningImage[i] = NULL;

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
	switch (event->getType ())
	{
		case EVENT_ALL_PROCESSED:
			queObs (*(int *)(event->getArg ()));
			break;
	}
#ifdef RTS2_HAVE_PGSQL
	rts2db::DeviceDb::postEvent (event);
#else
	rts2core::Device::postEvent (event);
#endif
}

int ImageProc::getFreeSlot ()
{
	int free_slot = -1;
	int np = numProc->getValueInteger ();
	for (int i = 0; i < np; i++)
		if (!runningImage[i])
		{
			free_slot = i;
			break;
		}
	return free_slot;
}

int ImageProc::idle ()
{
	if (imagesQue.size () != 0)
	{
		int free_slot = getFreeSlot ();

		if (free_slot >= 0)
		{
			std::list < ConnProcess * >::iterator img_iter = imagesQue.begin ();
			ConnProcess *newImage = *img_iter;
			imagesQue.erase (img_iter);
			changeRunning (newImage, free_slot);
		}
	}

#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::idle ();
#else
	return rts2core::Device::idle ();
#endif
}

int ImageProc::info ()
{
	queSize->setValueInteger ((int) imagesQue.size () + numRunning ());
	sendValueAll (queSize);

	struct statvfs s;
	statvfs(rts2core::Configuration::instance ()->observatoryBasePath ().c_str (), &s);

	freeDiskSpace->setValueDouble (s.f_bavail*s.f_frsize);
	sendValueAll (freeDiskSpace);

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
					globPos = 0;
				}
				reprocessingPossible = 0;
				break;
			}
		default:
			if (strlen (image_glob->getValue ()))
			{
				reprocessingPossible = 1;
				if (getFreeSlot () >= 0 && imagesQue.size () == 0)
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
	// Find the runningImage slot corresponding to the connection
	int slot = -1;
	int np = numProc->getValueInteger ();

	for (int i = 0; i < np; i++)
		if (runningImage[i] == conn){
			slot = i;
			break;
		}

	std::list < ConnProcess * >::iterator img_iter;
	ConnProcess *rImage = NULL;

	if (slot >= 0)
	{
		rImage = runningImage[slot];

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
		queSize->setValueInteger (imagesQue.size () + numRunning () - 1); // Here we count all running slots except this one
		sendValueAll (queSize);
		if (rImage)
			rImage->deleteConnection (conn);
	}

	if (conn == rImage)
	{
		// que next image
		// rts2core::Device::deleteConnection will delete rImage
		switch (rImage->getAstrometryStat ())
		{
			case GET:
				goodImages->inc ();
				nightGoodImages->inc ();
				lastRaDec->setValueRaDec (((ConnImgOnlyProcess *) rImage)->getRa (), ((ConnImgOnlyProcess *) rImage)->getDec ());
				lastCorrections->setValueRaDec (((ConnImgOnlyProcess *) rImage)->getRaErr (), ((ConnImgOnlyProcess *) rImage)->getDecErr ());
				sendValueAll (goodImages);
				sendValueAll (nightGoodImages);
				sendValueAll (lastRaDec);
				sendValueAll (lastCorrections);
				if (std::isnan (lastGood->getValueDouble ()) || rImage->getExposureEnd () > lastGood->getValueDouble ())
				{
					lastGood->setValueDouble (rImage->getExposureEnd ());
					sendValueAll (lastGood);
				}
				break;
			case NOT_ASTROMETRY:
			case TRASH:
				trashImages->inc ();
				nightTrashImages->inc ();
				sendValueAll (trashImages);
				sendValueAll (nightTrashImages);
				if (std::isnan (lastTrash->getValueDouble ()) || rImage->getExposureEnd () > lastTrash->getValueDouble ())
				{
					lastTrash->setValueDouble (rImage->getExposureEnd ());
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
			case OBS:
				numObs->inc ();
				sendValueAll (numObs);
				break;
			default:
				logStream (MESSAGE_ERROR) << "wrong image state: " << rImage->getAstrometryStat () << sendLog;
				break;
		}
		runningImage[slot] = NULL;
		img_iter = imagesQue.begin ();
		if (img_iter != imagesQue.end ())
		{
			ConnProcess *cp = *img_iter;
			imagesQue.erase (img_iter);
			changeRunning (cp, slot);
		}
		// still not image process running..
		if (runningImage[slot] == NULL)
		{
			if (numRunning () == 0)
				maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_IDLE);

			if (reprocessingPossible)
			{
				queNextFromGlob();
			}
		}
	}
#ifdef RTS2_HAVE_PGSQL
	return rts2db::DeviceDb::deleteConnection (conn);
#else
	return rts2core::Device::deleteConnection (conn);
#endif
}

void ImageProc::changeRunning (ConnProcess * newImage, int slot)
{
	int ret;

	if (runningImage[slot])
	{
		if (sendStop)
		{
			runningImage[slot]->stop ();
			imagesQue.push_front (runningImage[slot]);
		}
		else
		{
			imagesQue.push_front (newImage);
			infoAll ();
			return;
		}
	}

	runningImage[slot] = newImage;
	runningImage[slot]->setConnectionDebug (getDebug ());
#ifdef RTS2_HAVE_LIBJPEG
		runningImage[slot]->setLastProcessedJpeg (last_processed_jpeg);
#endif
	ret = runningImage[slot]->init ();

	if (ret < 0)
	{
		deleteConnection (runningImage[slot]);
		runningImage[slot] = NULL;
		if (numRunning () == 0)
			maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, DEVICE_ERROR_HW | IMGPROC_IDLE);
		else
			maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, DEVICE_ERROR_HW | IMGPROC_RUN);
		infoAll ();
		if (reprocessingPossible)
			checkNotProcessed ();
		return;
	}
	else if (ret == 0)
	{
#ifdef RTS2_HAVE_LIBJPEG
		if (std::isnan (lastGood->getValueDouble ()) || lastGood->getValueDouble () < runningImage[slot]->getExposureEnd ())
			runningImage[slot]->setLastGoodJpeg (last_good_jpeg);
		if (std::isnan (lastGood->getValueDouble ()) || lastTrash->getValueDouble() < runningImage[slot]->getExposureEnd ())
			runningImage[slot]->setLastTrashJpeg (last_trash_jpeg);
#endif
		addConnection (runningImage[slot]);
		processedImage->setValueCharArr (runningImage[slot]->getProcessArguments ());
	}
	maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_RUN);
	infoAll ();
}

int ImageProc::que (ConnProcess * newProc)
{
	int slot = getFreeSlot ();
	if (slot < 0)
		imagesQue.push_front (newProc);
	else
		changeRunning (newProc, slot);
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
	int slot = getFreeSlot ();
	if (slot < 0)
		slot = numProc->getValueInteger () - 1; // stop the probably most recently started one
	changeRunning (newImageConn, slot);
	infoAll ();
	return 0;
}

int ImageProc::queObs (int obsId)
{
	if (access (defaultObsProcess.c_str (), X_OK) == 0)
	{
		ConnObsProcess *newObsConn = new ConnObsProcess (this, defaultObsProcess.c_str (), obsId, astrometryTimeout->getValueInteger ());
		return que (newObsConn);
	}
	else
		return 0;
}

int ImageProc::queNextFromGlob ()
{
	if (imageGlob.gl_pathc == 0)
		return 0;

	while (globPos < imageGlob.gl_pathc && getFreeSlot() >= 0)
	{
		// It is better to double check whether this file is already being processed by other worker slots
		bool alreadyProcessing = false;

		for (int i = 0; i < numProc->getValueInteger (); i++)
			if (runningImage[i] && !strcmp(runningImage[i]->getProcessArguments(), imageGlob.gl_pathv[globPos]))
			{
				alreadyProcessing = true;
				break;
			}

		if (!alreadyProcessing)
		{
			queImage (imageGlob.gl_pathv[globPos]);
		}

		globPos++;
	}

	if (globPos >= imageGlob.gl_pathc)
	{
		globfree (&imageGlob);
		imageGlob.gl_pathc = 0;
		globPos = 0;
	}

	return 1;
}

int ImageProc::numRunning ()
{
	int num = 0;

	for (int i = 0; i < numProc->getValueInteger (); i++)
		if (runningImage[i] != NULL)
			num++;

	return num;
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

	globPos = 0;

	// start files queue and fill all free worker slots
	for (int i = 0; i < imageGlob.gl_pathc && getFreeSlot() >= 0; i++)
		queNextFromGlob();

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
	else if (conn->isCommand("reprocess")) // Re-process unprocessed images from queue dir
	{
		if (strlen (image_glob->getValue ()))
		{
			logStream (MESSAGE_INFO) << "Initiating re-processing of " << image_glob->getValue () << sendLog;

			reprocessingPossible = 1;
			if (getFreeSlot () >= 0 && imagesQue.size () == 0)
				checkNotProcessed ();
		}
		return 0;
	}
	else if (conn->isCommand("stop_reprocess")) // Re-process unprocessed images from queue dir
	{
		logStream (MESSAGE_INFO) << "Stopping re-processing of " << image_glob->getValue () << sendLog;

		globfree (&imageGlob);
		imageGlob.gl_pathc = 0;
		globPos = 0;
		return 0;
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
