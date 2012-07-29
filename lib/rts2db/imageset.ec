/* 
 * Set of images.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/imageset.h"
#include "rts2db/observation.h"
#include "rts2fits/dbfilters.h"

#include <sstream>

#include "rts2fits/imagedb.h"

using namespace rts2db;

ImageSet::ImageSet ()
{
}

ImageSet::~ImageSet (void)
{
	std::vector <rts2image::Image *>::iterator img_iter;
	for (img_iter = begin (); img_iter != end (); img_iter++)
	{
		delete *img_iter;
	}
	clear ();
}

int ImageSet::load (std::string in_where)
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
	int d_filter_id;
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
	VARCHAR d_img_path[101];

	int d_img_temperature_ind;
	int d_img_err_ra_ind;
	int d_img_err_dec_ind;
	int d_img_err_ind;
	int d_img_path_ind;

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
		"filter_id,"
		"img_alt,"
		"img_az,"
		"camera_name,"
		"mount_name,"
		"delete_flag,"
		"process_bitfield,"
		"img_err_ra,"
		"img_err_dec,"
		"img_err,"
		"img_path"
		" FROM "
		"images,"
		"observations"
		" WHERE "
		" images.obs_id = observations.obs_id AND " << in_where <<
		" ORDER BY "
		"img_date ASC;";

	stmp_c = new char[_os.str ().length () + 1];
	strcpy (stmp_c, _os.str ().c_str ());

	EXEC SQL PREPARE cur_images_stmp FROM :stmp_c;

	delete[] stmp_c;

	rts2image::DBFilters *filters = rts2image::DBFilters::instance ();
	filters->load ();

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
				:d_filter_id,
				:d_img_alt,
				:d_img_az,
				:d_camera_name,
				:d_mount_name,
				:d_delete_flag,
				:d_process_bitfield,
				:d_img_err_ra :d_img_err_ra_ind,
				:d_img_err_dec :d_img_err_dec_ind,
				:d_img_err :d_img_err_ind,
				:d_img_path :d_img_path_ind;
		if (sqlca.sqlcode)
			break;

		if (d_img_temperature_ind < 0)
			d_img_temperature = NAN;
		if (d_img_err_ra_ind < 0)
			d_img_err_ra = NAN;
		if (d_img_err_dec_ind < 0)
			d_img_err_dec = NAN;
		if (d_img_err_ind < 0)
			d_img_err = NAN;

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

		std::vector <ImageSetStat>::iterator iter = getStat (d_filter_id);

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

		d_camera_name.arr[d_camera_name.len] = '\0';
		d_mount_name.arr[d_mount_name.len] = '\0';
		if (d_img_path_ind < 0)
			d_img_path.arr[0] = '\0';
		else
			d_img_path.arr[d_img_path.len] = '\0';

		push_back (new rts2image::ImageSkyDb (d_tar_id, d_obs_id, d_img_id, d_obs_subtype,
			d_img_date, d_img_usec, d_img_exposure, d_img_temperature, (*filters)[d_filter_id].c_str (), d_img_alt, d_img_az,
			d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
			d_img_err_dec, d_img_err, d_img_path.arr));

	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream(MESSAGE_ERROR) << "ImageSet::load error in DB: " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL CLOSE cur_images;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL CLOSE cur_images;
	EXEC SQL COMMIT;

	stat ();

	return 0;
}

void ImageSet::stat ()
{
	// compute statistics
	allStat.stat ();

	for (std::vector <ImageSetStat>::iterator iter = filterStat.begin (); iter != filterStat.end (); iter++)
	{
		(*iter).stat();
	}
}

void ImageSet::print (std::ostream &_os, int printImages)
{
	if ((printImages & DISPLAY_ALL) || (printImages & DISPLAY_FILENAME))
	{
		std::vector <rts2image::Image *>::iterator img_iter;
		if (empty () && !(printImages & DISPLAY_SHORT))
		{
			_os << "      " << "--- no images ---" << std::endl;
			return;
		}
		for (img_iter = begin (); img_iter != end (); img_iter++)
		{
			rts2image::Image *image = (*img_iter);
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

int ImageSet::getAverageErrors (double &eRa, double &eDec, double &eRad)
{
	double tRa;
	double tDec;
	double tRad;

	eRa = 0;
	eDec = 0;
	eRad = 0;

	int aNum = 0;

	std::vector <rts2image::Image *>::iterator img_iter;
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

std::vector <ImageSetStat>::iterator ImageSet::getStat (int in_filter)
{
	std::vector <ImageSetStat>::iterator iter;
	for (iter = filterStat.begin (); iter != filterStat.end (); iter++)
	{
		if ((*iter).filter == in_filter)
			return iter;
	}
	filterStat.push_back (ImageSetStat (in_filter));
	return --(filterStat.end());
}


ImageSetTarget::ImageSetTarget (int in_tar_id)
{
	tar_id = in_tar_id;
}

int ImageSetTarget::load ()
{
	std::ostringstream os;
	os << "tar_id = " << tar_id;
	return ImageSet::load (os.str());
}

ImageSetObs::ImageSetObs (Observation *in_observation)
{
	observation = in_observation;
}

int ImageSetObs::load ()
{
	std::ostringstream os;
	os << "observations.obs_id = " << observation->getObsId ();
	return ImageSet::load (os.str());
}

ImageSetPosition::ImageSetPosition (struct ln_equ_posn * in_pos)
{
	pos = *in_pos;
}

int ImageSetPosition::load ()
{
	std::ostringstream os;
	os << "isinwcs (" << pos.ra
		<< ", " << pos.dec
		<< ", astrometry)";
	return ImageSet::load (os.str ());
}

int ImageSetDate::load ()
{
	std::ostringstream _os;
	_os << " observations.obs_slew >= to_timestamp (" << from << ")"
	 	<< " AND observations.obs_slew <= to_timestamp ("
		<< to
		<< ") AND (observations.obs_end is NULL OR observations.obs_end < to_timestamp ("
		<< to
			<< "))";
	return ImageSet::load (_os.str ());
}
