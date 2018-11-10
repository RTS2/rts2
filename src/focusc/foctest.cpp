/* 
 * Test focusing client.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2fits/image.h"

#include <unistd.h>
#include <iostream>

int main (int argc, char **argv)
{
	if (argc < 2 || !argv[1])
	{
		std::cout << "Don't get image!" << std::endl;
		return 1;
	}
	rts2image::Image *image;
	long naxes[2];
	image = new rts2image::Image ();
	image->openFile (argv[1]);
	std::cout << "average: " << image->getAverage () << std::endl;
	image->getValues ("NAXIS", naxes, 2);
	std::cout << "NAXIS: " << naxes[0] << "x" << naxes[1] << std::endl;
	sleep (10);
	std::cout << "change 132 10" << std::endl;
	return 0;
}
