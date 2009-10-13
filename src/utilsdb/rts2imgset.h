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

#ifndef __RTS2_IMGSET__
#define __RTS2_IMGSET__

#include "../writers/rts2image.h"

#include <iostream>
#include <vector>
#include "rts2imgsetstat.h"

class Rts2Obs;
class Rts2ObsSet;
class Rts2ImageDb;

/**
 * Class for accesing images.
 *
 * Compute statistics on images, allow easy access to images,..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ImgSet:public std::vector <Rts2Image * >
{
	private:
		Rts2ImgSetStat allStat;

		// which images filters are in set..
		std::vector <Rts2ImgSetStat> filterStat;

	protected:
		int load (std::string in_where);
		void stat ();
	public:
		Rts2ImgSet ();
		// abstract. subclasses needs to define that
		virtual int load () = 0;
		virtual ~Rts2ImgSet (void);
		void print (std::ostream & _os, int printImages);
		int getAverageErrors (double &eRa, double &eDec, double &eRad);

		std::vector < Rts2ImgSetStat >::iterator   getStat (std::string in_filter);

		friend std::ostream & operator << (std::ostream & _os, Rts2ImgSet & img_set);
};

class Rts2ImgSetTarget:public Rts2ImgSet
{
	private:
		int tar_id;
	public:
		Rts2ImgSetTarget (int in_tar_id);
		virtual int load ();
};

class Rts2ImgSetObs:public Rts2ImgSet
{
	private:
		Rts2Obs *observation;
	public:
		Rts2ImgSetObs (Rts2Obs * in_observation);
		virtual int load ();
};

class Rts2ImgSetPosition:public Rts2ImgSet
{
	private:
		struct ln_equ_posn pos;
	public:
		Rts2ImgSetPosition (struct ln_equ_posn *in_pos);
		virtual int load ();
};

std::ostream & operator << (std::ostream & _os, Rts2ImgSet & img_set);
#endif							 /* !__RTS2_IMGSET__ */
