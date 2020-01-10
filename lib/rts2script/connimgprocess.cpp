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

#include "rts2script/connimgprocess.h"

#include "command.h"
#include "configuration.h"
#include "utilsfunc.h"

#include "rts2script/script.h"
#include "rts2db/taruser.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

using namespace rts2plan;
using namespace rts2image;

ConnProcess::ConnProcess (rts2core::Block * in_master, const char *in_exe, int in_timeout):rts2script::ConnExe (in_master, in_exe, false, in_timeout)
{
	astrometryStat = NOT_ASTROMETRY;
}

int ConnProcess::init ()
{
	if (exePath[0] == '\0')
		return -1;
	return ConnExe::init ();
}

ConnImgOnlyProcess::ConnImgOnlyProcess (rts2core::Block *_master, const char *_exe, const char *_path, int _timeout):ConnProcess (_master, _exe, _timeout)
{
	imgPath = std::string (_path);
	addArg (imgPath);
}

int ConnImgOnlyProcess::init ()
{
	try
	{
		Image image;
		image.openFile (imgPath.c_str (), true, false);
		if (image.getShutter () == SHUT_CLOSED)
		{
			astrometryStat = DARK;
			return 0;
		}

		expDate = image.getExposureStart () + image.getExposureLength ();
	}
	catch (rts2core::Error &e)
	{
		int ret = unlink (imgPath.c_str ());
		if (ret)
			logStream (MESSAGE_ERROR) << "error removing " << imgPath << ":" << strerror (errno) << sendLog;
		else
			logStream (MESSAGE_WARNING) << "removed " << imgPath << sendLog;
		astrometryStat = BAD;
		return -2;
	}
	return ConnProcess::init ();
}

void ConnImgOnlyProcess::processCommand (char *cmd)
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
			checkAstrometry ();
		}
	}
	else if (!strcasecmp (cmd, "corrwerr"))
	{
		double err;
		if (paramNextLong (&id) || paramNextDouble (&ra) || paramNextDouble (&dec)
			|| paramNextDouble (&ra_err) || paramNextDouble (&dec_err) || paramNextDouble (&err) || !paramEnd ())
		{
			logStream (MESSAGE_WARNING) << "invalid corrwerr string" << sendLog;
		}
		else
		{
			astrometryStat = GET;
			checkAstrometry ();
		}
	}
	else
	{
		ConnProcess::processCommand (cmd);
	}
}

void ConnImgOnlyProcess::processLine ()
{
	int ret;
	ret = sscanf (getCommand (), "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err, &dec_err);

	if (ret == 5)
	{
		ra_err /= 60.0;
		dec_err /= 60.0;
		astrometryStat = GET;
		checkAstrometry ();
	}
	else
	{
		ConnProcess::processLine ();
	}

	return;
}

void ConnImgOnlyProcess::connectionError (int last_data_size)
{
	if (astrometryStat == NOT_ASTROMETRY)
		astrometryStat = BAD;
	ConnProcess::connectionError (last_data_size);
}

ConnImgProcess::ConnImgProcess (rts2core::Block *_master, const char *_exe, const char *_path, int _timeout, int _end_event):ConnImgOnlyProcess (_master, _exe, _path, _timeout)
{
	end_event = _end_event;
}

int ConnImgProcess::init ()
{
#ifdef RTS2_HAVE_LIBJPEG
	try
	{
		Image image;

		image.openFile (imgPath.c_str (), true, false);

		if (!last_processed_jpeg.empty ())
		{
			image.writeAsJPEG (last_processed_jpeg, 1, "%Y-%m-%d %H:%M:%S @OBJECT");
		}
	}
	catch (rts2core::Error &e)
	{
	}
#endif

	return ConnImgOnlyProcess::init ();
}


int ConnImgProcess::newProcess ()
{
	if (astrometryStat == DARK)
		return 0;

	return ConnImgOnlyProcess::newProcess ();
}

void ConnImgProcess::connectionError (int last_data_size)
{
	const char *telescopeName;
	int corr_mark, corr_img, corr_obs;

	if (last_data_size < 0 && errno == EAGAIN)
	{
		logStream (MESSAGE_DEBUG) << "ConnImgProcess::connectionError " << strerror (errno) << " #" << errno << " last_data_size " << last_data_size << sendLog;
		return;
	}

#ifdef RTS2_HAVE_PGSQL
	ImageDb *image;
	try
	{
		ImageDb *imagedb = new ImageDb ();
		imagedb->openFile (imgPath.c_str ());
		image = getValueImageType (imagedb);
#else
	Image *image = NULL;
	try
	{
		image = new Image ();
		image->openFile (imgPath.c_str ());
#endif
		if (image->getImageType () == IMGTYPE_FLAT || image->getImageType () == IMGTYPE_DARK)
		{
			// just return..
			if (image->getImageType () == IMGTYPE_FLAT)
			{
				astrometryStat = FLAT;
				if (end_event <= 0)
					image->toFlat ();
			}
			else
			{
				astrometryStat = DARK;
				if (end_event <= 0)
					image->toDark ();
			}
			delete image;
			ConnImgOnlyProcess::connectionError (last_data_size);
			return;
		}

		switch (astrometryStat)
		{
			case NOT_ASTROMETRY:
			case TRASH:
#ifdef RTS2_HAVE_LIBJPEG
				if (!last_trash_jpeg.empty ())
					image->writeAsJPEG (last_trash_jpeg, 1, "%Y-%m-%d %H:%M:%S @OBJECT");
#endif
				astrometryStat = TRASH;
				if (end_event <= 0)
					image->toTrash ();
				break;
			case GET:
#ifdef RTS2_HAVE_LIBJPEG
				if (!last_good_jpeg.empty ())
					image->writeAsJPEG (last_good_jpeg, 1, "%Y-%m-%d %H:%M:%S @OBJECT");
#endif
				image->setAstroResults (ra, dec, ra_err, dec_err);
				if (end_event <= 0)
					image->toArchive ();

				if (ra_err != 0 || dec_err != 0)
				{
					// send correction information to executor, to be sent to telescope when it is safe
					telescopeName = image->getMountName ();
					try
					{
						image->getValue ("MOVE_NUM", corr_mark);
						image->getValue ("CORR_IMG", corr_img);
						image->getValue ("CORR_OBS", corr_obs);

						if (telescopeName)
						{
							rts2core::Connection *execConn = master->getOpenConnection (DEVICE_TYPE_EXECUTOR);
							rts2core::ValueBool *apply_correction = (rts2core::ValueBool *) ((rts2core::Daemon *) master)->getOwnValue ("apply_corrections");

							if (execConn && Configuration::instance ()->isAstrometryDevice (image->getCameraName ()) && (apply_correction == NULL || apply_correction->getValueBool ()))
							{
								struct ln_equ_posn pos1, pos2;
								pos1.ra = ra;
								pos1.dec = dec;

								pos2.ra = ra - ra_err;
								pos2.dec = dec - dec_err;

								double posErr = ln_get_angular_separation (&pos1, &pos2);

								std::ostringstream _os;
								_os << "correction_info " << telescopeName << " "
									<< corr_mark << " " << corr_img << " " << corr_obs << " "
									<< image->getImgId () << " " << image->getObsId () << " "
									<< ra_err << " " << dec_err << " " << posErr;

								execConn->queCommand (new rts2core::Command (master, _os));
							}
						}
					}
					catch (rts2image::KeyNotFound &er)
					{
					}
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
			master->postEvent (new rts2core::Event (end_event, (void *) image));
		}
		else
		{
			if (astrometryStat == GET)
				master->postEvent (new rts2core::Event (EVENT_OK_ASTROMETRY, (void *) image));
			else
				master->postEvent (new rts2core::Event (EVENT_NOT_ASTROMETRY, (void *) image));
		}

#ifdef RTS2_HAVE_PGSQL
		// Check whether it was the last image of the observation
		int obsId = image->getObsId ();
		rts2db::Observation *obs = new rts2db::Observation (obsId);

		if (obs->checkUnprocessedImages (master) == 0)
			master->postEvent (new rts2core::Event (EVENT_ALL_PROCESSED, (void *) &obsId));

		delete obs;
#endif

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
		ConnImgOnlyProcess::connectionError (last_data_size);
		return;
	}

	ConnImgOnlyProcess::connectionError (last_data_size);
}

void ConnImgOnlyProcess::checkAstrometry ()
{
	if (ra_err > 180)
		ra_err -= 360;
	if (ra_err < -180)
		ra_err += 360;
	if (fabs (ra_err) > 180)
	{
		logStream (MESSAGE_ERROR) << "received invalid ra error: " << ra_err << " " << LibnovaDegDist (ra_err) << sendLog;
		astrometryStat = BAD;
	}
	if (fabs (dec_err) >= 180)
	{
		logStream (MESSAGE_ERROR) << "received invalid dec error: " << dec_err << " " << LibnovaDegDist (dec_err) << sendLog;
		astrometryStat = BAD;
	}
}

ConnObsProcess::ConnObsProcess (rts2core::Block * in_master, const char *in_exe, int in_obsId, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
	astrometryStat = OBS;

#ifdef RTS2_HAVE_PGSQL
	obsId = in_obsId;

	addArg (obsId);

	obs = new rts2db::Observation (obsId);
	if (obs->load ())
	{
		logStream (MESSAGE_ERROR) << "ConnObsProcess::ConnObsProcess cannot load obs " << obsId << sendLog;
		addArg (-1);
		addArg ('-');
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "ConnObsProcess::ConnObsProcess loaded obs " << obsId << sendLog;
		addArg (obs->getTargetId ());
		addArg (obs->getTargetType ());
	}
#endif
}

ConnObsProcess::~ConnObsProcess (void)
{
	delete obs;
}

void ConnObsProcess::processLine ()
{
	// no error
	return;
}
