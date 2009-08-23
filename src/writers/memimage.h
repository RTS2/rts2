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

#ifndef __RTS2_MEMIMAGE__
#define __RTS2_MEMIMAGE__

#include "rts2image.h"

namespace rts2image
{

/**
 * Creates memory only image. Delete it when image is deleted.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MemImage:public Rts2Image
{
	public:
		/**
		 * Creates memory only image.
		 *
		 * @param timeval Start of the esposure.
		 */
		MemImage (const struct timeval *_exposureStart);
		virtual ~MemImage ();

	protected:
		virtual int createFile ();

	private:
		void *imgbuf;
		size_t imgsize;
};

}

#endif // !__RTS2_MEMIMAGE__
