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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

using namespace rts2plan;

ConnProcess::ConnProcess (Rts2Block * in_master, const char *in_exe, int in_timeout):rts2script::ConnExe (in_master, in_exe, false, in_timeout)
{
	astrometryStat = NOT_ASTROMETRY;

#ifdef HAVE_LIBJPEG
	last_good_jpeg = NULL;
	last_trash_jpeg = NULL;
#endif
}

ConnImgProcess::ConnImgProcess (Rts2Block *_master, const char *_exe, const char *_path, int _timeout, int _end_event):ConnProcess (_master, _exe, _timeout)
{
	end_event = _end_event;
	imgPath = std::string (_path);
	addArg (imgPath);
}

ConnImgProcess::~ConnImgProcess (void)
{
}

int ConnImgProcess::init ()
{
	try
	{
		Rts2Image image;
		image.openImage (imgPath.c_str (), false, true);
		if (image.getShutter () == SHUT_CLOSED)
		{
			astrometryStat = DARK;
			return 0;
		}

		expDate = image.getExposureStart () + image.getExposureLength ();
	}
	catch (rts2core::Error &e)
	{
		logStream (MESSAGE_ERROR) << "error processing " << imgPath.c_str () << " :" << e << sendLog;
		return -2;
	}
	return ConnProcess::init ();
}

int ConnImgProcess::newProcess ()
{
	if (astrometryStat == DARK)
		return 0;
	
	return rts2script::ConnExe::newProcess ();
}

void ConnImgProcess::processCommand (char *cmd)
{
	if (!strcasecmp (cmd, "correct"))
	{
		if (paramNextLong (&id) || paramNextDouble (&ra) || paramNextDouble (&dec)
			|| paramNextDouble (&ra_err) || paramNextDouble (&dec_err) || !paramEnd ())
		{
			logStream (MESSAGE_WARNING) << "invalid correct string" << sendLog;
		}
		else
		{
			astrometryStat = GET;
		}
	}
	else
	{
		rts2script::ConnExe::processCommand (cmd);
	}
}

void ConnImgProcess::processLine ()
{
	int ret;
	ret = sscanf (getCommand (), "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err, &dec_err);

	if (ret == 5)
	{
	 	ra_err /= 60.0;
		dec_err /= 60.0;
		astrometryStat = GET;
		// inform others..
	}
	else
	{
		ConnExe::processLine ();
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
	  	Rts2ImageDb *imagedb = new Rts2ImageDb ();
		imagedb->openImage (imgPath.c_str ());
		image = getValueImageType (imagedb);
#else
	Rts2Image *image = NULL;
	try
	{
		image = new Rts2Image ();
		image->openImage (imgPath.c_str ());
#endif
		if (image->getImageType () == IMGTYPE_FLAT || image->getImageType () == IMGTYPE_DARK)
		{
			// just return..
			if (image->getImageType () == IMGTYPE_FLAT)
				astrometryStat = FLAT;
			else
			  	astrometryStat = DARK;
			delete image;
			rts2script::ConnExe::connectionError (last_data_size);
			return;
		}

		switch (astrometryStat)
		{
			case NOT_ASTROMETRY:
			case TRASH:
#ifdef HAVE_LIBJPEG
				if (last_trash_jpeg)
					image->writeAsJPEG (last_trash_jpeg, "%Y-%m-%d %H:%M:%S @OBJECT");
#endif
				astrometryStat = TRASH;
				if (end_event <= 0)
					image->toTrash ();
				break;
			case GET:
#ifdef HAVE_LIBJPEG
				if (last_good_jpeg)
					image->writeAsJPEG (last_good_jpeg, "%Y-%m-%d %H:%M:%S @OBJECT");
#endif

				image->setAstroResults (ra, dec, ra_err, dec_err);
				if (end_event <= 0)
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

							pos2.ra = ra - ra_err;
							pos2.dec = dec - dec_err;

							double posErr = ln_get_angular_separation (&pos1, &pos2);

							telConn->queCommand (new Rts2CommandCorrect (master, corr_mark,	
								corr_img, image->getImgId (), ra_err, dec_err, posErr)
							);
						}
					}
				}
				catch (rts2image::KeyNotFound &er)
				{
				}
				break;
			case DARK:
				if (end_event <= 0)
					image->toDark ();
				break;
			default:
				break;
		}
		if (end_event > 0)
		{
			master->postEvent (new Rts2Event (end_event, (void *) image));
		}
		else
		{
			if (astrometryStat == GET)
				master->postEvent (new Rts2Event (EVENT_OK_ASTROMETRY, (void *) image));
			else
				master->postEvent (new Rts2Event (EVENT_NOT_ASTROMETRY, (void *) image));
		}
		delete image;
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "Processing " << imgPath << ": " << er << sendLog;
		//delete image;
		// move file to bad directory..

		int i = 0;
	
		for (std::string::iterator iter = imgPath.end () - 1; iter != imgPath.begin (); iter--)
		{
			if (*iter == '/')
			{
				i = iter - imgPath.begin ();
				break;
			}
		}

		std::string newPath = imgPath.substr (0, i) + std::string ("/bad/") + imgPath.substr (i + 1);

		int ret = mkpath (newPath.c_str (), 0777);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot create path for file: " << newPath << ":" << strerror (errno) << sendLog;
		}
		else
		{
			ret = rename (imgPath.c_str (), newPath.c_str ());
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
		rts2script::ConnExe::connectionError (last_data_size);
		return;
	}

	rts2script::ConnExe::connectionError (last_data_size);
}

ConnObsProcess::ConnObsProcess (Rts2Block * in_master, const char *in_exe, int in_obsId, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
#ifdef HAVE_PGSQL
	obsId = in_obsId;
	obs = new rts2db::Observation (obsId);
	if (obs->load ())
	{
		logStream (MESSAGE_ERROR) << "ConnObsProcess::newProcess cannot load obs " << obsId << sendLog;
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
