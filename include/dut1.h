#ifndef __RTS2_DUT1__
#define __RTS2_DUT1__
/* 
 * Parse DUT1 file and output DUT1 for given date.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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
 */

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves DUT from filename stored on HDD for given GM date.
 * File with offsets can be downloaded from: 
 * http://maia.usno.navy.mil/ser7/finals2000A.daily
 *
 * @param fn         filename
 * @param gmdate     date for which offset should be retrieved
 * @return DUT1 for given data, NAN if DUT was not found in the provided file
 */
double getDUT1 (const char *fn, struct tm *gmdate);

#ifdef __cplusplus
};
#endif

#endif //!__RTS2_DUT1__
