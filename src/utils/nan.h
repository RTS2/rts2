/* 
 * Various utility functions.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS_NAN__
#define __RTS_NAN__

#include <config.h>

#ifndef HAVE_ISINF
#include <ieeefp.h>
#endif

#if defined(__WIN32__) || defined(sun) || defined(__C89_SUB__)

/* Not a Number function generator */
double rts2_nan (const char *code);

#else

#define rts2_nan(f)  nan(f)

#endif /* defined(__WIN32__) || defined(sun) || defined(__C89_SUB__) */

#endif	 /* !__RTS_NAN__ */
