/* 
 * Set of images.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "imgdisplay.h"
#include "rts2imgset.h"
#include "rts2obs.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include "../writers/rts2imagedb.h"

Rts2ImgSet::Rts2ImgSet ()
{
}


Rts2ImgSet::~Rts2ImgSet (void)
{
	std::vector <Rts2Image *>::iterator img_iter;
	for (img_iter = begin (); img_iter != end (); img_iter++)
	{
		delete *img_iter;
	}
	clear ();
}


int
Rts2ImgSet::load (std::string in_where)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char *stmp_c;

		int d_tar_id;
		int d_obs_id;
		int d_img_id;
		char d_obs_subtype;
		long d_img_date;
		int d_img_usec;
		float d_img_exposure;
		float d_img_temperature;
		VARCHAR d_img_filter[3];
		float d_img_alt;
		float d_img_az;
		// cannot use DEVICE_NAME_SIZE, as some versions of ecpg complains about it
		VARCHAR d_camera_name[50];
		// cannot use DEVICE_NAME_SIZE, as some versions of ecpg complains about it
		VARCHAR d_mount_name[50];
		bool d_delete_flag;
		int d_process_bitfield;
		double d_img_err_ra;
		double d_img_err_dec;
		double d_img_err;

		int d_img_temperature_ind;
		int d_img_err_ra_ind;
		int d_img_err_dec_ind;
		int d_img_err_ind;

	EXEC SQL END DECLARE SECTION;

	std::ostringstream _os;
	
	_os << "SELECT "
		"tar_id,"
		"img_id,"
		"images.obs_id,"
		"obs_subtype,"
		"EXTRACT (EPOCH FROM img_date),"
		"img_usec,"
		"img_exposure,"
		"img_temperature,"
		"img_filter,"
		"img_alt,"
		"img_az,"
		"camera_name,"
		"mount_name,"
		"delete_flag,"
		"process_bitfield,"
		"img_err_ra,"
		"img_err_dec,"
		"img_err"
		" FROM "
		"images,"
		"observations"
		" WHERE "
		" images.obs_id = observations.obs_id AND " << in_where <<
		" ORDER BY "
		"img_id ASC;";

	stmp_c = new char[_os.str ().length () + 1];
	strcpy (stmp_c, _os.str ().c_str ());

	EXEC SQL PREPARE cur_images_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE cur_images CURSOR FOR cur_images_stmp;

	EXEC SQL OPEN cur_images;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_images INTO
				:d_tar_id,
				:d_img_id,
				:d_obs_id,
				:d_obs_subtype,
				:d_img_date,
				:d_img_usec,
				:d_img_exposure,
				:d_img_temperature :d_img_temperature_ind,
				:d_img_filter,
				:d_img_alt,
				:d_img_az,
				:d_camera_name,
				:d_mount_name,
				:d_delete_flag,
				:d_process_bitfield,
				:d_img_err_ra :d_img_err_ra_ind,
				:d_img_err_dec :d_img_err_dec_ind,
				:d_img_err :d_img_err_ind;
		if (sqlca.sqlcode)
			break;

		d_img_filter.arr[d_img_filter.len] = '\0';

		if (d_img_temperature_ind < 0)
			d_img_temperature = nan ("f");
		if (d_img_err_ra_ind < 0)
			d_img_err_ra = nan ("f");
		if (d_img_err_dec_ind < 0)
			d_img_err_dec = nan ("f");
		if (d_img_err_ind < 0)
			d_img_err = nan ("f");

		allStat.img_alt += d_img_alt;
		allStat.img_az  += d_img_az;
		if (!isnan (d_img_err))
		{
			allStat.img_err += d_img_err;
			allStat.img_err_ra  += d_img_err_ra;
			allStat.img_err_dec += d_img_err_dec;
			allStat.astro_count++;
		}
		allStat.count++;
		allStat.exposure += d_img_exposure;

		std::vector <Rts2ImgSetStat>::iterator iter = getStat (std::string (d_img_filter.arr));

		(*iter).img_alt += d_img_alt;
		(*iter).img_az  += d_img_az;
		if (!isnan (d_img_err))
		{
			(*iter).img_err += d_img_err;
			(*iter).img_err_ra  += d_img_err_ra;
			(*iter).img_err_dec += d_img_err_dec;
			(*iter).astro_count++;
		}
		(*iter).count++;
		(*iter).exposure += d_img_exposure;

		push_back (new Rts2ImageSkyDb (d_tar_id, d_obs_id, d_img_id, d_obs_subtype,
			d_img_date, d_img_usec, d_img_exposure, d_img_temperature, d_img_filter.arr, d_img_alt, d_img_az,
			d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
			d_img_err_dec, d_img_err));

	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream(MESSAGE_ERROR) << "Rts2ImgSet::load error in DB: " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL CLOSE cur_images;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL CLOSE cur_images;
	EXEC SQL COMMIT;

	stat ();

	return 0;
}


void
Rts2ImgSet::stat ()
{
	// compute statistics
	allStat.stat ();

	for (std::vector <Rts2ImgSetStat>::iterator iter = filterStat.begin (); iter != filterStat.end (); iter++)
	{
		(*iter).stat();
	}
}


void
Rts2ImgSet::print (std::ostream &_os, int printImages)
{
	if ((printImages & DISPLAY_ALL) || (printImages & DISPLAY_FILENAME))
	{
		std::vector <Rts2Image *>::iterator img_iter;
		if (empty () && !(printImages & DISPLAY_SHORT))
		{
			_os << "      " << "--- no images ---" << std::endl;
			return;
		}
		for (img_iter = begin (); img_iter != end (); img_iter++)
		{
			Rts2Image *image = (*img_iter);
			if ((printImages & DISPLAY_ASTR_OK) && !image->haveOKAstrometry ())
				continue;
			if ((printImages & DISPLAY_ASTR_TRASH) && (image->haveOKAstrometry () || !image->isProcessed ()))
				continue;
			if ((printImages & DISPLAY_ASTR_QUE) && image->isProcessed ())
				continue;
			image->print (_os, printImages);
		}
	}
	if (printImages & DISPLAY_SUMMARY)
	{
		_os << *this << std::endl;
	}
}


int
Rts2ImgSet::getAverageErrors (double &eRa, double &eDec, double &eRad)
{
	double tRa;
	double tDec;
	double tRad;

	eRa = 0;
	eDec = 0;
	eRad = 0;

	int aNum = 0;

	std::vector <Rts2Image *>::iterator img_iter;
	for (img_iter = begin (); img_iter != end (); img_iter++)
	{
		if (((*img_iter)->getError (tRa, tDec, tRad)) == 0)
		{
			eRa += tRa;
			eDec += tDec;
			eRad += tRad;
			aNum++;
		}
	}
	if (aNum > 0)
	{
		eRa /= aNum;
		eDec /= aNum;
		eRad /= aNum;
	}
	return aNum;
}


std::vector <Rts2ImgSetStat>::iterator
Rts2ImgSet::getStat (std::string in_filter)
{
	std::vector <Rts2ImgSetStat>::iterator iter;
	for (iter = filterStat.begin ();
		iter != filterStat.end (); iter++)
	{
		if ((*iter).filter == in_filter)
			return iter;
	}
	filterStat.push_back (Rts2ImgSetStat (in_filter));
	return --(filterStat.end());
}


Rts2ImgSetTarget::Rts2ImgSetTarget (int in_tar_id)
{
	tar_id = in_tar_id;
}


int
Rts2ImgSetTarget::load ()
{
	std::ostringstream os;
	os << "tar_id = " << tar_id;
	return Rts2ImgSet::load (os.str());
}


Rts2ImgSetObs::Rts2ImgSetObs (Rts2Obs *in_observation)
{
	observation = in_observation;
}


int
Rts2ImgSetObs::load ()
{
	std::ostringstream os;
	os << "observations.obs_id = " << observation->getObsId ();
	return Rts2ImgSet::load (os.str());
}


Rts2ImgSetPosition::Rts2ImgSetPosition (struct ln_equ_posn * in_pos)
{
	pos = *in_pos;
}


int
Rts2ImgSetPosition::load ()
{
	std::ostringstream os;
	os << "isinwcs (" << pos.ra
		<< ", " << pos.dec
		<< ", astrometry)";
	return Rts2ImgSet::load (os.str ());
}


Rts2ImgSetFlats::Rts2ImgSetFlats (Rts2ObsSet *in_observations)
{
	observations = in_observations;
}


int
Rts2ImgSetFlats::load ()
{
	return -1;
}


Rts2ImgSetDarks::Rts2ImgSetDarks (Rts2ObsSet *in_observations)
{
	observations = in_observations;
}


int
Rts2ImgSetDarks::load ()
{
	return -1;
}


std::ostream & operator << (std::ostream &_os, Rts2ImgSet &img_set)
{
	_os << "Filter  all#             exposure good# ( %%%)    avg. err     avg. alt      avg. az" << std::endl;
	for (std::vector <Rts2ImgSetStat>::iterator iter = img_set.filterStat.begin (); iter != img_set.filterStat.end (); iter++)
	{
		Rts2ImgSetStat stat = *iter;
		_os << std::setw (5) << stat.filter << " " << stat  << std::endl;
	}
	_os << "Total " << img_set.allStat;
	return _os;
}
