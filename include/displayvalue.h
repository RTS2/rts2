/* 
 * Configuration file read routines.
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

#ifndef __RTS2_DISPLAYVALUE__
#define __RTS2_DISPLAYVALUE__

#include "value.h"
#include "libnova_cpp.h"

namespace rts2core
{

/**
 * Get value as string usefull for displaying it. It does all the pretty
 * printing conversions, using value flags.
 *
 * @param value  Value which will be displayed.
 *
 * @return String which contains variable informations.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
std::string getDisplayValue (rts2core::Value * value, bool print_milisec = true);

}
#endif							 /* !__RTS2_DISPLAYVALUE__ */
