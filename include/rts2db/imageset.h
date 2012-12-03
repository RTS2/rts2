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

#ifndef __RTS2_IMGSET__
#define __RTS2_IMGSET__

#include "../rts2fits/image.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include "imagesetstat.h"

class Rts2ImageDb;

namespace rts2db {

class Observation;

/**
 * Class for accesing images.
 *
 * Compute statistics on images, allow easy access to images,..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ImageSet:public std::vector <rts2image::Image * >
{
	public:
		ImageSet ();
		// abstract. subclasses needs to define that
		virtual int load () = 0;
		virtual ~ImageSet (void);
		void print (std::ostream & _os, int printImages);
		int getAverageErrors (double &eRa, double &eDec, double &eRad);

		const ImageSetStat getAllStat ();

		std::vector < ImageSetStat >::iterator getStat (int in_filter);

		friend std::ostream & operator << (std::ostream & _os, ImageSet & img_set)
		{
			_os << "Filter  all#             exposure good# ( %%%)    avg. err     avg. alt      avg. az" << std::endl;
			for (std::vector <ImageSetStat>::iterator iter = img_set.filterStat.begin (); iter != img_set.filterStat.end (); iter++)
			{
				ImageSetStat stat = *iter;
				_os << std::setw (5) << stat.filter << " " << stat  << std::endl;
			}
			_os << "Total " << img_set.allStat;
			return _os;
		}
	protected:
		int load (std::string in_where);
		void stat ();
	private:
		ImageSetStat allStat;

		// which images filters are in set..
		std::vector <ImageSetStat> filterStat;
};

class ImageSetTarget:public ImageSet
{
	public:
		ImageSetTarget (int in_tar_id);
		virtual int load ();
	private:
		int tar_id;
};

class ImageSetObs:public ImageSet
{
	public:
		ImageSetObs (rts2db::Observation * in_observation);
		virtual int load ();
	private:
		rts2db::Observation *observation;
};

class ImageSetPosition:public ImageSet
{
	public:
		ImageSetPosition (struct ln_equ_posn *in_pos);
		virtual int load ();
	private:
		struct ln_equ_posn pos;
};

class ImageSetDate:public ImageSet
{
	public:
		ImageSetDate (time_t _from, time_t _to) { from = _from; to = _to; }
		virtual int load ();
	private:
		time_t from;
		time_t to;
};

class ImageSetLabel:public ImageSet
{
	public:
		ImageSetLabel (int label_id) { label = label_id; }
		virtual int load ();
	private:
		int label;
};

}
#endif							 /* !__RTS2_IMGSET__ */
