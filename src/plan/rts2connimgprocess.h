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

#ifndef __RTS2CONNIMGPROCESS__
#define __RTS2CONNIMGPROCESS__

#include "../utils/rts2connfork.h"
#include "../writers/rts2imagedb.h"
#include "../utilsdb/rts2obs.h"

typedef enum
{ NOT_ASTROMETRY, TRASH, GET, MORNING, DARK, FLAT }
astrometry_stat_t;

class Rts2ConnProcess:public Rts2ConnFork
{
	protected:
		astrometry_stat_t astrometryStat;
	public:
		Rts2ConnProcess (Rts2Block * in_master,
			const char *in_exe, int in_timeout);

		astrometry_stat_t getAstrometryStat ()
		{
			return astrometryStat;
		}
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
 */
class Rts2ConnImgProcess:public Rts2ConnProcess
{
	private:
		char *imgPath;

		long id;
		double ra, dec, ra_err, dec_err;

		void sendOKMail (Rts2ImageDb * image);
		void sendProcEndMail (Rts2ImageDb * image);

	protected:
		virtual void connectionError (int last_data_size);

	public:
		Rts2ConnImgProcess (Rts2Block * in_master,
			const char *in_exe, const char *in_path,
			int in_timeout);
		virtual ~ Rts2ConnImgProcess (void);

		virtual int newProcess ();
		virtual int processLine ();
};

class Rts2ConnObsProcess:public Rts2ConnProcess
{
	private:
		int obsId;
		Rts2Obs *obs;

		char *obsIdCh;
		char *obsTarIdCh;
		char *obsTarTypeCh;
	public:
		Rts2ConnObsProcess (Rts2Block * in_master,
			const char *in_exe, int in_obsId, int in_timeout);

		virtual int newProcess ();
		virtual int processLine ();
};

class Rts2ConnDarkProcess:public Rts2ConnProcess
{
	public:
		Rts2ConnDarkProcess (Rts2Block * in_master,
			const char *in_exe, int in_timeout);

		virtual int processLine ();
};

class Rts2ConnFlatProcess:public Rts2ConnProcess
{
	public:
		Rts2ConnFlatProcess (Rts2Block * in_master,
			const char *in_exe, int in_timeout);

		virtual int processLine ();
};
#endif							 /* !__RTS2CONNIMGPROCESS__ */
