#define _GNU_SOURCE

#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../utils/config.h"
#include "../utils/devcli.h"
#include "../utils/mkpath.h"
#include "../utils/mv.h"
#include "../writers/fits.h"
#include "../db/db.h"
#include "process_image.h"

#define EPOCH			"002"

struct image_que
{
  char *image;
  char *directory;
  int correction_mark;
  char *tel_name;
  struct image_que *previous;
}
 *image_que;

pthread_mutex_t image_que_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t image_que_cond = PTHREAD_COND_INITIALIZER;

/*!
 * Handle image processing
 */
void *
process_images (void *arg)
{
  struct image_que *actual_image;

  struct device *tel;

  char *cmd, *filename;

  long int id;
  double ra, dec, ra_err, dec_err;
  time_t t = time (NULL);

  int ret = 0;

  FILE *past_out;
  FILE *cor_log;

  image_que = NULL;

  cor_log = fopen ("/home/petr/tel_move_log", "a");
  fprintf (cor_log, "===== new log %s", ctime (&t));

  while (1)
    {
      // wait for image to process
      pthread_mutex_lock (&image_que_mutex);
      while (!image_que)
	pthread_cond_wait (&image_que_cond, &image_que_mutex);
      actual_image = image_que;
      image_que = image_que->previous;
      pthread_mutex_unlock (&image_que_mutex);

      printf ("chdir %s\n", actual_image->directory);

      if ((ret = chdir (actual_image->directory)))
	{
	  perror ("chdir");
	  goto free_image;
	}

      // call image processing script
      asprintf (&cmd,
		get_string_default ("astrometry",
				    "/home/petr/rts2/src/plan/img_process %s 2>/dev/null"),
		actual_image->image);
      printf ("\ncalling %s.", cmd);

      if (!(past_out = popen (cmd, "r")))
	{
	  perror ("popen");
	  free (cmd);
	  ret = -1;
	  goto free_image;
	};

      free (cmd);

      printf ("OK\n");

      do
	{
	  char strs[200];
	  if (!fgets (strs, 200, past_out))
	    {
	      ret = EOF;
	      break;
	    }

	  strs[199] = 0;

	  ret = sscanf
	    (strs, "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err,
	     &dec_err);
	}
      while (!(ret == EOF || ret == 5));

      pclose (past_out);
      filename =
	(char *) malloc (strlen (actual_image->directory) +
			 strlen (actual_image->image) + 1);
      strcpy (filename, actual_image->directory);
      strcat (filename, actual_image->image);

      if (ret == EOF)
	{
	  // bad image, move to trash
	  char *trash_name;
	  trash_name = (char *) malloc (8 + strlen (filename));
	  sprintf (trash_name, "/trash%s", actual_image->directory);
	  mkpath (trash_name, 0777);
	  strcat (trash_name, actual_image->image);

	  printf ("%s scanf error, invalid line\n", filename);
	  printf ("mv %s -> %s", filename, trash_name);
	  if ((ret = (mv (filename, trash_name))))
	    perror ("rename bad image");
	  free (trash_name);
	  printf ("..OK\n");
	}
      else
	{

	  printf ("ra: %f dec: %f ra_err: %f min dec_err: %f min\n", ra, dec,
		  ra_err, dec_err);

	  if (actual_image->correction_mark >= 0)
	    {
	      fprintf (cor_log, "%s %f %f %f %f\n", actual_image->image, ra,
		       dec, ra_err, dec_err);
	      fflush (cor_log);
	      if ((tel = devcli_find (actual_image->tel_name)))
		{
		  if (devcli_command (tel, NULL, "correct %i %f %f",
				      actual_image->correction_mark,
				      ra_err / 60.0, dec_err / 60.0))
		    perror ("telescope correct");
		}
	      else
		{
		  fprintf (stderr, "**** cannot find telescope name %s\n",
			   actual_image->tel_name);
		}
	    }

	  // add image to db
	  asprintf (&cmd,
		    "cd /images && /home/petr/fitsdb/wcs/wcs2db %s|psql stars",
		    filename + 8);

	  if (system (cmd))
	    {
	      perror ("calling wcs2db");
	    }

	  free (cmd);
	}

      free (filename);

    free_image:
      free (actual_image->directory);
      free (actual_image->image);
      free (actual_image);
    }
}

/*!
 * Handle camera data connection.
 *
 * @params sock 	socket fd.
 *
 * @return	0 on success, -1 and set errno on error.
 */
int
data_handler (int sock, size_t size, struct image_info *image)
{
  struct fits_receiver_data receiver;
  struct tm gmt;
  char data[DATA_BLOCK_SIZE];
  int ret = 0;
  ssize_t s;
  char *filen;
  char *filename;
  char *dirname;
  char *dark_name;

  gmtime_r (&image->exposure_time, &gmt);

  printf ("camera_name : %s\n", image->camera_name);

  switch (image->target_type)
    {
    case TARGET_DARK:
      asprintf (&dirname, "/darks/%s/%s/%04i/%02i/", EPOCH,
		image->camera_name, gmt.tm_year + 1900, gmt.tm_mon + 1);
      break;

    case TARGET_FLAT:
      asprintf (&dirname, "/flats/%s/%s/%04i/%02i/%02i/", EPOCH,
		image->camera_name, gmt.tm_year + 1900, gmt.tm_mon + 1,
		gmt.tm_mday);
      break;

    case TARGET_FLAT_DARK:
      asprintf (&dirname, "/flats/%s/%s/%04i/%02i/%02i/darks/", EPOCH,
		image->camera_name, gmt.tm_year + 1900, gmt.tm_mon + 1,
		gmt.tm_mday);
      break;

    default:
      asprintf
	(&dirname, "/images/%s/%s/%05i/", EPOCH, image->camera_name,
	 image->target_id);
    }

  mkpath (dirname, 0777);

  filen = (char *) malloc (25);

  strftime (filen, 24, "%Y%m%d%H%M%S.fits", &gmt);
  filen[24] = 0;
  asprintf (&filename, "%s%s", dirname, filen);
  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      printf ("camc data_handler fits_init\n");
      ret = -1;
      goto free_filen;
    }
  printf ("reading data socket: %i size: %i\n", sock, size);

  while ((s = devcli_read_data (sock, data, DATA_BLOCK_SIZE)) > 0)
    {
      if (s < 0)
	{
	  printf ("socket error: %i", sock);
	  goto close_fits;
	}

      if ((ret = fits_handler (data, s, &receiver)) < 0)
	goto close_fits;
      if (ret == 1)
	break;
    }

  if (s < 0)
    {
      perror ("camc data_handler");
      ret = -1;
      goto close_fits;
    }

  printf ("reading finished\n");

  if (image->target_type == TARGET_LIGHT)
    db_get_darkfield (image->camera_name, image->exposure_length * 100,
		      image->camera.ccd_temperature * 10, &dark_name);

  if (fits_write_image_info (&receiver, image, dark_name)
      || fits_close (&receiver))
    {
      perror ("camc data_handler fits_write");
      ret = -1;
      free (dark_name);
      goto close_fits;
    }

  free (dark_name);

  switch (image->target_type)
    {
      struct image_que *new_que;
    case TARGET_LIGHT:
      printf ("putting %s to que\n", filename);

      new_que = (struct image_que *) malloc (sizeof (struct image_que));
      new_que->directory = dirname;
      new_que->image = filen;
      new_que->tel_name = image->telescope_name;

      if (strcmp (image->camera_name, "C0") == 0)
	new_que->correction_mark = image->telescope.correction_mark;
      else
	new_que->correction_mark = -1;

      pthread_mutex_lock (&image_que_mutex);
      new_que->previous = image_que;
      image_que = new_que;
      pthread_cond_broadcast (&image_que_cond);
      pthread_mutex_unlock (&image_que_mutex);

      ret = 0;

      goto free_filename;
    case TARGET_DARK:
      printf ("darkname: %s\n", filename);

      ret = db_add_darkfield (filename, &image->exposure_time,
			      image->exposure_length * 100,
			      image->camera.ccd_temperature * 10,
			      image->camera_name);
      printf ("after db_add_darkfield\n");
      fflush (stdout);
      goto free_filen;

    case TARGET_FLAT:
      printf ("flatname: %s\n", filename);
      goto free_filen;

    case TARGET_FLAT_DARK:
      printf ("dark flatname: %s\n", filename);
      goto free_filen;
    }

close_fits:
  fits_close (&receiver);

free_filen:
  free (filen);
  free (dirname);

free_filename:
  free (filename);
  return ret;
#undef receiver
}
