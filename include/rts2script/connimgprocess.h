/*
 * Connections for image processing forked instances.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010-2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2CONNIMGPROCESS__
#define __RTS2CONNIMGPROCESS__

#include "connexe.h"
#include "rts2fits/imagedb.h"
#include "rts2db/observation.h"

namespace rts2plan
{

typedef enum { NOT_ASTROMETRY, TRASH, GET, DARK, BAD, FLAT, OBS } astrometry_stat_t;

class ConnProcess:public rts2script::ConnExe
{
	public:
		ConnProcess (rts2core::Block * in_master, const char *in_exe, int in_timeout);

		virtual int init ();

		astrometry_stat_t getAstrometryStat () { return astrometryStat; }

		double getExposureEnd () { return expDate; };

		/**
		 * Return name of the image/thing to process.
		 */
		virtual const char* getProcessArguments () { return "none"; }

#ifdef RTS2_HAVE_LIBJPEG
		void setLastGoodJpeg (std::string _last_good_jpeg) { last_good_jpeg = _last_good_jpeg; }
		void setLastTrashJpeg (std::string _last_trash_jpeg) { last_trash_jpeg = _last_trash_jpeg; }
#endif

	protected:
		astrometry_stat_t astrometryStat;
		double expDate;

#ifdef RTS2_HAVE_LIBJPEG
		std::string last_good_jpeg;
		std::string last_trash_jpeg;
#endif
};

/**
 * Only process the image, do not move it, do not try to distinguish if it's
 * dark - don't use any RTS2 image operation functions.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class ConnImgOnlyProcess:public ConnProcess
{
	public:
		/**
		 *
		 * @param _end_event  If set to value > 0, this event will be emmited at the end of image processing, with image passed
		 *	as argument. Then the standard image processing - bad to trash, with astrometry to archive - will not be run.
		 */
		ConnImgOnlyProcess (rts2core::Block *_master, const char *_exe, const char *_path, int _timeout);

		virtual int init ();

		virtual const char* getProcessArguments () { return imgPath.c_str (); }

		virtual void processLine ();

		double getRa () { return ra; }
		double getDec () { return dec; }

		double getRaErr () { return ra_err; }
		double getDecErr () { return dec_err; }

	protected:
		virtual void processCommand (char *cmd);

		virtual void connectionError (int last_data_size);

		std::string imgPath;

		long id;
		double ra, dec, ra_err, dec_err;

	private:
		// normalize astrometry output and check if astrometry errors are reasonable
		void checkAstrometry ();
};

/**
 * "Connection" which reads output of image processor
 *
 * This class expect that images are stored in CENTRAL repository,
 * accesible throught NFS/other network sharing to all machines on
 * which imgproc runs.
 *
 * Hence passing full image path will be sufficient for finding
 * it.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnImgProcess:public ConnImgOnlyProcess
{
	public:
		/**
		 *
		 * @param _end_event  If set to value > 0, this event will be emmited at the end of image processing, with image passed
		 *	as argument. Then the standard image processing - bad to trash, with astrometry to archive - will not be run.
		 */
		ConnImgProcess (rts2core::Block *_master, const char *_exe, const char *_path, int _timeout, int _end_event = -1);

		virtual int newProcess ();

	protected:
		virtual void connectionError (int last_data_size);

	private:
		int end_event;
};

class ConnObsProcess:public ConnProcess
{
	public:
		ConnObsProcess (rts2core::Block * in_master, const char *in_exe, int in_obsId, int in_timeout);

		virtual const char* getProcessArguments () { return obs == NULL ? "unknown" : obs->getTargetName ().c_str (); }

		virtual void processLine ();

	private:
		int obsId;
		rts2db::Observation *obs;
};

};
#endif							 /* !__RTS2CONNIMGPROCESS__ */
