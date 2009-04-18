/* 
 * Various utility functions.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS_UTILSFUNC__
#define __RTS_UTILSFUNC__

#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/time.h>
#include <sstream>

#include <string.h>

#include "config.h"

#ifndef HAVE_ISINF
#include <ieeefp.h>
#endif
/**
 * Create directory recursively.
 *
 * @param path Path that will be created.
 * @param mode Create mask for path creation.
 *
 * @return 0 on success, otherwise error code.
 */
int mkpath (const char *path, mode_t mode);

/**
 * Split std::string to vector of strings.
 *
 * @param text        String which will be splitted.
 * @param delimeter   String shich separates entries in vector.
 *
 * @return Vector of std::string.
 */
std::vector<std::string> SplitStr (const std::string& text, const std::string& delimeter);

/**
 * Splits string to vector of chars
 *
 * @param text   Text which will be splited.
 */
std::vector<char> Str2CharVector (std::string text);


/**
 * Fill value to const char**.
 *
 * @param p    Pointer to char which will be filled,
 * @param val  Value which will be copied to character.
 */
template < typename T >
void fillIn (char **p, T val)
{
	std::ostringstream _os;
	_os << val;
	*p = new char[_os.str ().length () + 1];
	strcpy (*p, _os.str (). c_str ());
}

/**
 * Replacement for isinf - on Solaris platform
 */
#ifndef HAVE_ISINF
int isinf(double x);
#endif

#ifndef timerisset
#define timerisset(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifndef timerclear
#define timerclear(tvp)	((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

#ifndef timercmp
#define timercmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_usec CMP (b)->tv_usec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
#endif

#ifndef timeradd
#define timeradd(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;			      \
    if ((result)->tv_usec >= 1000000)					      \
      {									      \
	++(result)->tv_sec;						      \
	(result)->tv_usec -= 1000000;					      \
      }									      \
  } while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)
#endif

#endif							 /* !__RTS_UTILSFUNC__ */
