#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "mkpath.h"

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int
mkpath (const char *path, mode_t mode)
{
  char *cp_path;
  char *start, *end;
  int ret = 0;
  cp_path = strdup (path);
  start = cp_path;
  while (1)
    {
      end = ++start;
      while (*end && *end != '/')
	end++;
      if (!*end)
	break;
      *end = '\0';
      ret = mkdir (cp_path, mode);
      *end = '/';
      start = ++end;
      if (ret)
	{
	  if (errno != EEXIST)
	    break;
	  ret = 0;
	}
    }
  free (cp_path);
  return ret;
}
