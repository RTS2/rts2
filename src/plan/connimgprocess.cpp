/*
 * Connections for image processing forked instances.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "connimgprocess.h"
#include "script.h"

#include "../utils/rts2command.h"
#include "../utils/rts2config.h"
#include "../utils/utilsfunc.h"
#include "../utilsdb/rts2taruser.h"
#include "../utilsdb/rts2obs.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

using namespace rts2plan;

ConnProcess::ConnProcess (Rts2Block * in_master, const char *in_exe, int in_timeout):rts2core::ConnFork (in_master, in_exe, false, in_timeout)
{
}

ConnImgProcess::ConnImgProcess (Rts2Block * in_master, const char *in_exe, const char *in_path, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
	imgPath = new char[strlen (in_path) + 1];
	strcpy (imgPath, in_path);
	astrometryStat = NOT_ASTROMETRY;
}

ConnImgProcess::~ConnImgProcess (void)
{
	delete[]imgPath;
}

int ConnImgProcess::newProcess ()
{
	Rts2Image *image = NULL;

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "ConnImgProcess::newProcess exe: " <<
		exePath << " img: " << imgPath << " (" << getpid () << ")" << sendLog;
	#endif

	try
	{
		image = new Rts2Image (imgPath);
		image->openImage ();
		if (image->getShutter () == SHUT_CLOSED)
		{
			astrometryStat = DARK;
			delete image;
			return 0;
		}
		delete image;
	
		if (exePath)
		{
			execl (exePath, exePath, imgPath, (char *) NULL);
			logStream (MESSAGE_ERROR) << "ConnImgProcess::newProcess: " <<
				exePath << " " << imgPath << " " << strerror (errno) << sendLog;
		}
	}
	catch (rts2core::Error)
	{
		delete image;
		return -2;
	}
	return -2;
}

void ConnImgProcess::processLine ()
{
	int ret;
	ret = sscanf (getCommand (),
		"%li %lf %lf (%lf,%lf)",
		&id, &ra, &dec, &ra_err, &dec_err);

	if (ret == 5)
	{
		astrometryStat = GET;
		// inform others..
	}
	logStream (MESSAGE_DEBUG) << "received: " << getCommand () << " sscanf: "
		<< ret << sendLog;
	return;
}

void ConnImgProcess::connectionError (int last_data_size)
{
	const char *telescopeName;
	int corr_mark, corr_img;

	if (last_data_size < 0 && errno == EAGAIN)
	{
		logStream (MESSAGE_DEBUG) << "ConnImgProcess::connectionError " <<
			strerror (errno) << " #" << errno << " last_data_size " <<
			last_data_size << sendLog;
		return;
	}

#ifdef HAVE_PGSQL
	Rts2ImageDb *image;
	try
	{
		image = getValueImageType (new Rts2ImageDb (imgPath));
#else
	Rts2Image *image = NULL;
	try
	{
		image = new Rts2Image (imgPath);
#endif
		if (image->getImageType () == IMGTYPE_FLAT)
		{
			// just return..
			delete image;
			rts2core::ConnFork::connectionError (last_data_size);
			return;
		}

		switch (astrometryStat)
		{
			case NOT_ASTROMETRY:
			case TRASH:
				astrometryStat = TRASH;
				image->toTrash ();
				sendProcEndMail (image);
				break;
			case GET:
				image->setAstroResults (ra, dec, ra_err / 60.0, dec_err / 60.0);
				image->toArchive ();
				// send correction to telescope..
				telescopeName = image->getMountName ();
				try
				{
					image->getValue ("MOVE_NUM", corr_mark);
					image->getValue ("CORR_IMG", corr_img);
					if (telescopeName)
					{
						Rts2Conn *telConn;
						telConn = master->findName (telescopeName);
						// correction error should be in degrees
						if (telConn && Rts2Config::instance ()->isAstrometryDevice (image->getCameraName ()))
						{
							struct ln_equ_posn pos1, pos2;
							pos1.ra = ra;
							pos1.dec = dec;

							pos2.ra = ra - ra_err / 60.0;
							pos2.dec = dec - dec_err / 60.0;

							double posErr = ln_get_angular_separation (&pos1, &pos2);

							telConn->queCommand (new Rts2CommandCorrect (master, corr_mark,	
								corr_img, image->getImgId (), ra_err / 60.0, dec_err / 60.0, posErr)
							);
						}
					}
					sendProcEndMail (image);
				}
				catch (rts2image::KeyNotFound &er)
				{
				}
				break;
			case DARK:
				image->toDark ();
				break;
			default:
				break;
		}
		if (astrometryStat == GET)
			master->postEvent (new Rts2Event (EVENT_OK_ASTROMETRY, (void *) image));
		else
			master->postEvent (new Rts2Event (EVENT_NOT_ASTROMETRY, (void *) image));
		delete image;
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "Processing " << imgPath << ": " << er << sendLog;
		delete image;
		// move file to bad directory..
		char newPath[strlen(imgPath) + 5];
		char *last_slash = imgPath;
		char *p;
		while ((p = strchr (last_slash + 1, '/')) != NULL)
			last_slash = p + 1;
		if (last_slash != imgPath)
			last_slash--;
		strncpy (newPath, imgPath, last_slash - imgPath);
		strcpy (newPath + (last_slash - imgPath), "/bad");
		strcpy (newPath + (last_slash - imgPath) + 4, last_slash);

		int ret = mkpath (newPath, 0777);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot create path for file: " << newPath << ":" << strerror (errno) << sendLog;
		}
		else
		{
			ret = rename (imgPath, newPath);
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "Cannot rename " << imgPath << " to " << newPath << ":" << strerror(errno) << sendLog;
			}
			else
			{
				logStream (MESSAGE_INFO) << "Renamed " << imgPath << " to " << newPath << sendLog;
			}
		}
		astrometryStat = BAD;
		rts2core::ConnFork::connectionError (last_data_size);
		return;
	}

	rts2core::ConnFork::connectionError (last_data_size);
}

#ifdef HAVE_PGSQL
void ConnImgProcess::sendProcEndMail (Rts2ImageDb * image)
{
	int ret;
	int obsId;
	// last processed
	obsId = image->getObsId ();
	Rts2Obs observation = Rts2Obs (obsId);
	ret = observation.checkUnprocessedImages (master);
	if (ret == 0)
	{
		// que as
		getMaster ()->
			postEvent (new Rts2Event (EVENT_ALL_PROCESSED, (void *) &obsId));
	}
}
#endif

ConnObsProcess::ConnObsProcess (Rts2Block * in_master, const char *in_exe, int in_obsId, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
#ifdef HAVE_PGSQL
	obsId = in_obsId;
	obs = new Rts2Obs (obsId);
	if (obs->load ())
	{
		logStream (MESSAGE_ERROR) <<
			"ConnObsProcess::newProcess cannot load obs " << obsId << sendLog;
		obs = NULL;
	}

	fillIn (&obsIdCh, obsId);
	fillIn (&obsTarIdCh, obs->getTargetId ());
	fillIn (&obsTarTypeCh, obs->getTargetType ());

	delete obs;
#endif
}

int ConnObsProcess::newProcess ()
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "ConnObsProcess::newProcess exe: " <<
		exePath << " obsid: " << obsId << " pid: " << getpid () << sendLog;
	#endif

	if (exePath)
	{
		execl (exePath, exePath, obsIdCh, obsTarIdCh, obsTarTypeCh,
			(char *) NULL);
		// if we get there, it's error in execl
		logStream (MESSAGE_ERROR) << "ConnObsProcess::newProcess: " <<
			strerror (errno) << sendLog;
	}
	return -2;
}

void ConnObsProcess::processLine ()
{
	// no error
	return;
}

ConnDarkProcess::ConnDarkProcess (Rts2Block * in_master, const char *in_exe, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
}

void ConnDarkProcess::processLine ()
{
	return;
}

ConnFlatProcess::ConnFlatProcess (Rts2Block * in_master, const char *in_exe, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
}

void ConnFlatProcess::processLine ()
{
	return;
}
