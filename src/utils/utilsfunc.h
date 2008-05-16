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
 * @return Vector of std::string.
 */
std::vector<std::string> SplitStr(const std::string& text, const std::string& delimeter);

#endif							 /* !__RTS_UTILSFUNC__ */
