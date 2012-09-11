/* 
 * Support functions for directory listing.
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

#include <rts2-config.h>

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef RTS2_HAVE_SCANDIR
int scandir (const char *dirp, struct dirent ***namelist, int (*filter)(const struct dirent *), int (*compar)(const void *, const void *));
#endif // !RTS2_HAVE_SCANDIR

/**
 * Sort two file structure entries by cdate.
 */
#if (not (defined(_POSIX_C_SOURCE)) || _POSIX_C_SOURCE > 200200L) && defined(RTS2_HAVE_SCANDIR)
int cdatesort(const struct dirent **a, const struct dirent **b);
#else
int cdatesort(const void *a, const void *b);
#endif  // _POSIX_C_SOURCE

#ifndef RTS2_HAVE_ALPHASORT

#if _POSIX_C_SOURCE > 200200L && defined(RTS2_HAVE_SCANDIR)
int alphasort(const struct dirent **a, const struct dirent **b);
#else
int alphasort(const void *a, const void *b);
#endif // _POSIX_C_SOURCE

#endif // !RTS2_HAVE_ALPHASORT


