/*!
 * @author petr
 */

#define _GNU_SOURCE

#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

/*!
 * Test if path is directory
 */
int
isdir (const char *path)
{
  struct stat st;
  stat (path, &st);
  return S_ISDIR (st.st_mode);
}

int
create_dir (char *path, mode_t mode)
{
  if (mkdir (path, mode))
    {
      switch (errno)
	{
	case EEXIST:
	  if (!isdir (path))
	    {
	      errno = EEXIST;
	      return -1;
	    }
	  break;
	default:
#ifdef DEBUG
	  perror ("mkdir");
#endif /* DEBUG */
	  return -1;
	}
    }
  return 0;
}

/*!
 * Create path with given mode.
 *
 * @param pathname
 * @param mode
 *
 * Return 0 on sucess, -1 and set errno otherwise.
 */
int
mkpath (const char *path, mode_t mode)
{
  char *pathname, *tmp_path;

  if (!path)
    return -1;

  pathname = (char *) malloc (strlen (path));

  if (!pathname)
    return -1;
  strcpy (pathname, path);

  // don't start with first entry
  tmp_path = pathname + 1;

  // split path

  while (*tmp_path)
    {
      if (*tmp_path == '/')
	{
	  // found separator
	  *tmp_path = 0;

	  if (create_dir (pathname, mode))
	    return -1;
	  *tmp_path = '/';
	}
      tmp_path++;
    }

  if (create_dir (pathname, mode))
    return -1;
  return 0;
}
