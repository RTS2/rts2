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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "dirsupport.h"

#ifndef RTS2_HAVE_SCANDIR
int scandir (const char *dirp, struct dirent ***namelist, int (*filter)(const struct dirent *), int (*compar)(const void *, const void *))
{
	DIR *d = opendir (dirp);

	int nl_size = 20;
	*namelist = (struct dirent **) malloc (sizeof (dirent *) * 20);
	int nmeb = 0;

	int len = offsetof (struct dirent, d_name) + pathconf (dirp, _PC_NAME_MAX) + 1;

	struct dirent *dn = (struct dirent *) malloc (len);
	struct dirent *res = NULL;

	while (!readdir_r (d, dn, &res) && res != NULL)
	{
		if (!filter || filter (dn))
		{
			if (nl_size == 0)
			{
				*namelist = (struct dirent **) realloc (*namelist, sizeof (dirent *) * (nmeb + 20));
				nl_size = 20;
			}
			(*namelist)[nmeb] = dn;
			nmeb++;
			nl_size--;

			dn = (struct dirent *) malloc (len);
		}
	}

	closedir (d);

	if (*namelist)
		// sort it..
		qsort (*namelist, nmeb, sizeof (dirent *), compar);
	return nmeb;
}
#endif // !RTS2_HAVE_SCANDIR

/**
 * Sort two file structure entries by cdate.
 */
#if (not (defined(_POSIX_C_SOURCE)) || _POSIX_C_SOURCE > 200200L) && defined(RTS2_HAVE_SCANDIR)
int cdatesort(const struct dirent **a, const struct dirent **b)
{
	struct stat s_a, s_b;
        if (stat ((*a)->d_name, &s_a))
                return 1;
        if (stat ((*b)->d_name, &s_b))
                return -1;
        if (s_a.st_ctime == s_b.st_ctime)
                return 0;
        if (s_a.st_ctime > s_b.st_ctime)
                return 1;
        return -1;
}
#else
int cdatesort(const void *a, const void *b)
{
	struct stat s_a, s_b;
        const struct dirent * d_a = *((dirent**)a);
        const struct dirent * d_b = *((dirent**)b);
        if (stat (d_a->d_name, &s_a))
                return 1;
        if (stat (d_b->d_name, &s_b))
                return -1;
        if (s_a.st_ctime == s_b.st_ctime)
                return 0;
        if (s_a.st_ctime > s_b.st_ctime)
                return 1;
        return -1;
}
#endif  // _POSIX_C_SOURCE

#ifndef RTS2_HAVE_ALPHASORT

#if _POSIX_C_SOURCE > 200200L && defined(RTS2_HAVE_SCANDIR)
int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcmp ((*a)->d_name, (*b)->d_name);
}
#else
int alphasort(const void *a, const void *b)
{
        const struct dirent * d_a = *((dirent**)a);
        const struct dirent * d_b = *((dirent**)b);
	return strcmp (d_a->d_name, d_b->d_name);
}
#endif // _POSIX_C_SOURCE

#endif // !RTS2_HAVE_ALPHASORT


