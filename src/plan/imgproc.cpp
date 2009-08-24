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
#include "connimgprocess.h"
#include "rts2script.h"

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

#ifdef HAVE_PGSQL
#include "../utilsdb/rts2devicedb.h"
#else
#include "../utils/rts2device.h"
#include "../utils/rts2config.h"
#endif

namespace rts2plan
{

/**
 * Image processor main class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
#ifdef HAVE_PGSQL
class ImageProc:public Rts2DeviceDb
#else
class ImageProc:public Rts2Device
#endif
{
	private:
#ifndef HAVE_PGSQL
		const char *configFile;
#endif
		std::list < ConnProcess * >imagesQue;
		ConnProcess *runningImage;
		Rts2ValueInteger *goodImages;
		Rts2ValueInteger *trashImages;
		Rts2ValueInteger *badImages;

		Rts2ValueInteger *queSize;

		int sendStop;			 // if stop running astrometry with stop signal; it ussually doesn't work, so we will use FIFO
		std::string defaultImgProcess;
		std::string defaultObsProcess;
		std::string defaultDarkProcess;
		std::string defaultFlatProcess;
		glob_t imageGlob;
		unsigned int globC;
		int reprocessingPossible;
	protected:
		virtual int reloadConfig ();
#ifndef HAVE_PGSQL
		virtual int processOption (int opt);
		virtual int init ();
#endif
		virtual void signaledHUP ()
		{
			reloadConfig ();
		}
	public:
		ImageProc (int argc, char **argv);
		virtual ~ ImageProc (void);

		virtual void postEvent (Rts2Event * event);
		virtual int idle ();

		virtual int info ();

		virtual int changeMasterState (int new_state);

		virtual int deleteConnection (Rts2Conn * conn);

		int que (ConnProcess * newProc);

		int queImage (const char *in_path);
		int doImage (const char *in_path);

		int queFlat (const char *in_path);

		int queObs (int obsId);

		int queDarks ();
		int queFlats ();

		int checkNotProcessed ();
		void changeRunning (ConnProcess * newImage);

		virtual int commandAuthorized (Rts2Conn * conn);
};

};

using namespace rts2plan;

ImageProc::ImageProc (int _argc, char **_argv)
#ifdef HAVE_PGSQL
:Rts2DeviceDb (_argc, _argv, DEVICE_TYPE_IMGPROC, "IMGP")
#else
:Rts2Device (_argc, _argv, DEVICE_TYPE_IMGPROC, "IMGP")
#endif
{
	runningImage = NULL;

	createValue (goodImages, "good_images", "number of good images", false);
	goodImages->setValueInteger (0);

	createValue (trashImages, "trash_images", "number of images which ended in trash (bad images)", false);
	trashImages->setValueInteger (0);

	createValue (badImages, "bad_images", "number of bad images (in queue under bad directory)", false);
	badImages->setValueInteger (0);

	createValue (queSize, "que_size", "size of image que", false);

	imageGlob.gl_pathc = 0;
	imageGlob.gl_offs = 0;
	globC = 0;
	reprocessingPossible = 0;

	sendStop = 0;

#ifndef HAVE_PGSQL
	configFile = NULL;
	addOption (OPT_CONFIG, "config", 1, "configuration file");
#endif
}


ImageProc::~ImageProc (void)
{
	if (runningImage)
		delete runningImage;
	if (imageGlob.gl_pathc)
		globfree (&imageGlob);
}


int
ImageProc::reloadConfig ()
{
	int ret;
	
	Rts2Config *config;
#ifdef HAVE_PGSQL
	ret = Rts2DeviceDb::reloadConfig ();
	config = Rts2Config::instance ();
#else
	config = Rts2Config::instance ();
	ret = config->loadFile (configFile);
#endif
	if (ret)
		return ret;


	ret = config->getString ("imgproc", "astrometry", defaultImgProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"ImageProc::reloadConfig cannot get astrometry string, exiting!" <<
			sendLog;
		return ret;
	}

	ret = config->getString ("imgproc", "obsprocess", defaultObsProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"ImageProc::reloadConfig cannot get obs process script, exiting" <<
			sendLog;
	}

	ret = config->getString ("imgproc", "darkprocess", defaultDarkProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"ImageProc::reloadConfig cannot get dark process script, exiting" <<
			sendLog;
	}

	ret = config->getString ("imgproc", "flatprocess", defaultFlatProcess);
	if (ret)
	{
		logStream (MESSAGE_ERROR) <<
			"ImageProc::init cannot get flat process script, exiting" <<
			sendLog;
	}
	return ret;
}


#ifndef HAVE_PGSQL
int
ImageProc::processOption (int opt)
{
	switch (opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		default:
			return Rts2Device::processOption (opt);
	}
	return 0;
}

int
ImageProc::init ()
{
	int ret;
	ret = Rts2Device::init ();
	if (ret)
		return ret;
	return reloadConfig ();
}
#endif


void
ImageProc::postEvent (Rts2Event * event)
{
	int obsId;
	switch (event->getType ())
	{
		case EVENT_ALL_PROCESSED:
			obsId = *((int *) event->getArg ());
			queObs (obsId);
			break;
	}
#ifdef HAVE_PGSQL
	Rts2DeviceDb::postEvent (event);
#else
	Rts2Device::postEvent (event);
#endif
}


int
ImageProc::idle ()
{
	std::list < ConnProcess * >::iterator img_iter;
	if (!runningImage && imagesQue.size () != 0)
	{
		img_iter = imagesQue.begin ();
		ConnProcess *newImage = *img_iter;
		imagesQue.erase (img_iter);
		changeRunning (newImage);
	}
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::idle ();
#else
	return Rts2Device::idle ();
#endif
}


int
ImageProc::info ()
{
	queSize->setValueInteger ((int) imagesQue.size () + (runningImage ? 1 : 0));
	sendValueAll (queSize);
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::info ();
#else
	return Rts2Device::info ();
#endif
}


int
ImageProc::changeMasterState (int new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_DUSK:
		case SERVERD_DUSK | SERVERD_STANDBY_MASK:
		case SERVERD_NIGHT:
		case SERVERD_NIGHT | SERVERD_STANDBY_MASK:
		case SERVERD_DAWN:
		case SERVERD_DAWN | SERVERD_STANDBY_MASK:
			if (imageGlob.gl_pathc)
			{
				globfree (&imageGlob);
				imageGlob.gl_pathc = 0;
				globC = 0;
			}
			reprocessingPossible = 0;
			break;
		default:
			reprocessingPossible = 1;
			if (!runningImage && imagesQue.size () == 0)
				checkNotProcessed ();
	}
	// start dark & flat processing
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::changeMasterState (new_state);
#else
	return Rts2Device::changeMasterState (new_state);
#endif
}


int
ImageProc::deleteConnection (Rts2Conn * conn)
{
	std::list < ConnProcess * >::iterator img_iter;
	for (img_iter = imagesQue.begin (); img_iter != imagesQue.end ();
		img_iter++)
	{
		(*img_iter)->deleteConnection (conn);
		if (*img_iter == conn)
		{
			imagesQue.erase (img_iter);
		}
	}
	queSize->setValueInteger (imagesQue.size ());
	sendValueAll (queSize);
	if (runningImage)
		runningImage->deleteConnection (conn);
	if (conn == runningImage)
	{
		// que next image
		// Rts2Device::deleteConnection will delete runningImage
		switch (runningImage->getAstrometryStat ())
		{
			case GET:
				goodImages->inc ();
				sendValueAll (goodImages);
				break;
			case TRASH:
				trashImages->inc ();
				sendValueAll (trashImages);
				break;
			default:
				break;
		}
		runningImage = NULL;
		img_iter = imagesQue.begin ();
		if (img_iter != imagesQue.end ())
		{
			imagesQue.erase (img_iter);
			changeRunning (*img_iter);
		}
		else
		{
			maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_IDLE);
			if (reprocessingPossible)
			{
				if (globC < imageGlob.gl_pathc)
				{
					queImage (imageGlob.gl_pathv[globC]);
					globC++;
				}
				else if (imageGlob.gl_pathc > 0)
				{
					globfree (&imageGlob);
					imageGlob.gl_pathc = 0;
				}
			}
		}
	}
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::deleteConnection (conn);
#else
	return Rts2Device::deleteConnection (conn);
#endif
}


void
ImageProc::changeRunning (ConnProcess * newImage)
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
	ret = runningImage->init ();
	if (ret < 0)
	{
		delete runningImage;
		runningImage = NULL;
		maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN,
			DEVICE_ERROR_HW | IMGPROC_IDLE);
		infoAll ();
		return;
	}
	else if (ret == 0)
	{
		addConnection (runningImage);
	}
	maskState (DEVICE_ERROR_MASK | IMGPROC_MASK_RUN, IMGPROC_RUN);
	infoAll ();
}


int
ImageProc::que (ConnProcess * newProc)
{
	if (runningImage)
		imagesQue.push_front (newProc);
	else
		changeRunning (newProc);
	infoAll ();
	return 0;
}


int
ImageProc::queImage (const char *in_path)
{
	ConnImgProcess *newImageConn;
	newImageConn = new ConnImgProcess (this, defaultImgProcess.c_str (),
		in_path, Rts2Config::instance ()->getAstrometryTimeout ());
	return que (newImageConn);
}


int
ImageProc::doImage (const char *in_path)
{
	ConnImgProcess *newImageConn;
	newImageConn = new ConnImgProcess (this, defaultImgProcess.c_str (),
		in_path, Rts2Config::instance ()->getAstrometryTimeout ());
	changeRunning (newImageConn);
	infoAll ();
	return 0;
}


int
ImageProc::queFlat (const char *in_path)
{
	ConnImgProcess *newImageConn;
	newImageConn = new ConnImgProcess (this, defaultFlatProcess.c_str (),
		in_path, Rts2Config::instance ()->getAstrometryTimeout ());
	return que (newImageConn);
}


int
ImageProc::queObs (int obsId)
{
	ConnObsProcess *newObsConn;
	newObsConn = new ConnObsProcess (this, defaultObsProcess.c_str (),
		obsId, Rts2Config::instance ()->getObsProcessTimeout ());
	return que (newObsConn);
}


int
ImageProc::queDarks ()
{
	ConnDarkProcess *newDarkConn;
	newDarkConn = new ConnDarkProcess (this, defaultDarkProcess.c_str (),
		Rts2Config::instance ()->getDarkProcessTimeout ());
	return que (newDarkConn);
}


int
ImageProc::queFlats ()
{
	ConnFlatProcess *newFlatConn;
	newFlatConn = new ConnFlatProcess (this, defaultFlatProcess.c_str (),
		Rts2Config::instance ()->getFlatProcessTimeout ());
	return que (newFlatConn);
}


int
ImageProc::checkNotProcessed ()
{
	std::string image_glob;
	int ret;

	Rts2Config *config;
	config = Rts2Config::instance ();

	ret = config->getString ("imgproc", "imageglob", image_glob);
	if (ret)
		return ret;

	ret = glob (image_glob.c_str (), 0, NULL, &imageGlob);
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


int
ImageProc::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("que_image"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName) || !conn->paramEnd ())
			return -2;
		return queImage (in_imageName);
	}
	else if (conn->isCommand ("do_image"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName) || !conn->paramEnd ())
			return -2;
		return doImage (in_imageName);
	}
	else if (conn->isCommand ("que_flat"))
	{
		char *in_imageName;
		if (conn->paramNextString (&in_imageName) || !conn->paramEnd ())
			return -2;
		return queFlat (in_imageName);
	}
	else if (conn->isCommand ("que_obs"))
	{
		int obsId;
		if (conn->paramNextInteger (&obsId) || !conn->paramEnd ())
			return -2;
		return queObs (obsId);
	}
	else if (conn->isCommand ("que_darks"))
	{
		if (!conn->paramEnd ())
			return -2;
		return queDarks ();
	}
	else if (conn->isCommand ("que_flats"))
	{
		if (!conn->paramEnd ())
			return -2;
		return queFlats ();
	}
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::commandAuthorized (conn);
#else
	return Rts2Device::commandAuthorized (conn);
#endif
}


int
main (int argc, char **argv)
{
	ImageProc imgproc = ImageProc (argc, argv);
	return imgproc.run ();
}
