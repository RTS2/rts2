/*
 * Target from any string.
 * Copyright (C) 2005-2016 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGETRES__
#define __RTS2_TARGETRES__

#include "target.h"

/**
 * Return new target object, created from string. String might contain RA DEC pair, MPEC one-line or any Simbad or MPEC name.
 *
 * @param tar_string  String containing target name, RA DEC position, MPEC one-line or anything else that can be usefull to
 *               identify target.
 * @param debug Print debug data.
 *
 * @return new target object. Caller must deallocate target object (delete it).
 */
rts2db::Target *createTargetByString (std::string tar_string, bool debug);

#endif // __RTS2_TARGETRES__
