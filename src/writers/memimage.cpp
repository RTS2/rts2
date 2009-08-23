/* 
 * Class which represent memory only image.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "memimage.h"
#include <longnam.h>

using namespace rts2image;

MemImage::MemImage (const struct timeval *_exposureStart): Rts2Image ()
{
	createImage ();
	setExposureStart (_exposureStart);
	writeExposureStart ();
}

MemImage::~MemImage ()
{
}

int MemImage::createFile ()
{
	fitsfile *fp;
	imgsize = 2880;
	imgbuf = malloc (imgsize);
	fits_create_memfile (&fp, &imgbuf, &imgsize, 10 * imgsize, realloc, &fits_status);
	setFitsFile (fp);
	return 0;
}
