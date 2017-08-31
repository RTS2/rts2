/* 
 * Image header.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

/**
 * @file Image header.
 *
 * Standart image header. Used for transforming image header data between
 * camera deamon and camera client.
 *
 * It's quite necessary to have such a head, since condition on camera could
 * change unpredicitably after readout command was issuedddand client could
 * therefore receiver misleading information.
 *
 * That header is in no way mean as universal header for astronomical images.
 * It's only internal used among componnents of the rts software. It should
 * NOT be stored in permanent form, since it could change with versions of the
 * software.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#ifndef __RTS_IMGHDR__
#define __RTS_IMGHDR__

#include <time.h>

#ifndef sun
#include <stdint.h>
#endif /* !sun */

#define SHUTTER_OPEN     0x01
#define SHUTTER_CLOSED   0x02
#define SHUTTER_SYNCHRO  0x03

#define MAX_AXES            5	 //! Maximum number of axes we should considered.

// various datatypes
#define RTS2_DATA_BYTE       8
#define RTS2_DATA_SHORT     16
#define RTS2_DATA_LONG      32
#define RTS2_DATA_LONGLONG  64
#define RTS2_DATA_FLOAT    -32
#define RTS2_DATA_DOUBLE   -64

// unsigned data types
#define RTS2_DATA_SBYTE     10
#define RTS2_DATA_USHORT    20
#define RTS2_DATA_ULONG     40

#define RTS2_H_FLIPPED     0x01			//! horizontaly flip data before writing to FITS file
#define RTS2_V_FLIPPED     0x02			//! verticaly flip data before writing to FITS file

struct imghdr
{
	int16_t data_type;			//! date type - please see RTS2_DATA_XXXX constants
	int8_t flags;				//! Image flags - flipped axis,..
	int8_t naxes;				//! Number of axes.
	int32_t sizes[MAX_AXES];		//! Sizes in given axes.
	int16_t binnings[MAX_AXES];		//! Binning in each axe - eg. 2 -> 1 image pixel on given axis is equal 2 ccd pixels.
	int16_t filter;				//! Camera filter
	int16_t shutter;
	int16_t x, y;				//! image beginning (detector coordinates)
	uint16_t channel;			//! which channel is those data
};
#endif							 // __RTS_IMGHDR__
