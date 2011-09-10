/* 
 * NAN function.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "nan.h"

#if defined(__WIN32__) || defined(sun) || defined(__C89_SUB__)

/* Not a Number function generator */
double rts2_nan (const char *code)
{
        double zero = 0.0;

        return zero/0.0;
}

#endif /* defined(__WIN32__) || defined(sun) || defined(__C89_SUB__) */
