/*! @file
 * Defines mv function to move file.
 *
 * @author petr
 */

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE	1024

/*! Copy file to new location.
 *
 * @param old_name	file old name
 * @param new_name	file new name
 */
int
cp (char *old_name, char *new_name)
{
  int in, out;
  int readed;
  char buf[BUF_SIZE];
  struct stat old_stat;
  if ((in = open (old_name, O_RDONLY)) < 0 || stat (old_name, &old_stat))
    return -1;
  if ((out = open (new_name, O_WRONLY | O_CREAT)) < 0)
    return -1;

  while ((readed = read (in, &buf, BUF_SIZE)) > 0)
    {
      if (write (out, buf, readed) != readed)
	return -1;
    }

  close (out);
  close (in);

  chown (new_name, old_stat.st_uid, old_stat.st_gid);
  chmod (new_name, old_stat.st_mode);

  return 0;
}

/*! Moves file to new location.
 *
 * @param old_name	file old name
 * @param new_name	file new name
 */
int
mv (char *old_name, char *new_name)
{
  if (cp (old_name, new_name))
    return -1;
  if (unlink (old_name))
    {
      unlink (new_name);
      return -1;
    }
  return 0;
}
