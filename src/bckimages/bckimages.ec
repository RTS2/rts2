#define _GNU_SOURCE

#include "../db/db.h"
#include "../utils/mkpath.h"
#include "../utils/mv.h"

#include <getopt.h>
#include <glob.h>
#include <libgen.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int verbose = 0;
int do_move = 1;

struct move_files
{
  char *old_path;
  char *new_path;
};

int
move_images (char *epoch_id, int old_med, int new_med, int max_size)
{
  EXEC SQL BEGIN DECLARE SECTION;
  VARCHAR old_path[200];
  VARCHAR new_path[200];
  VARCHAR camera_name[8];
  VARCHAR mount_name[8];
  char *img_epoch_id = epoch_id;
  long int img_date;
  int img_new_med_id = new_med;
  int old_med_id = old_med;
  EXEC SQL END DECLARE SECTION;
  struct stat fst;
  struct move_files *mvf, *tmvf;
  int move_files_count = 1;
  char *new_path_str, *old_path_str;
  EXEC SQL BEGIN;

  EXEC SQL DECLARE images_to_move CURSOR FOR SELECT imgpath (med_id, epoch_id,
							     mount_name,
							     camera_name,
							     images.obs_id,
							     observations.
							     tar_id,
							     img_date),
    imgpath (:img_new_med_id, epoch_id, mount_name, camera_name,
	     images.obs_id, observations.tar_id, img_date), camera_name,
    mount_name, EXTRACT (EPOCH FROM img_date) FROM images,
    observations WHERE images.obs_id =
    observations.obs_id AND epoch_id =:img_epoch_id AND med_id =:old_med_id
    ORDER BY img_date ASC;

  EXEC SQL OPEN images_to_move;

  printf ("fetching rows..\n");

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM images_to_move
	INTO:old_path,:new_path,:camera_name,:mount_name,:img_date;
      if (sqlca.sqlcode)
	{
	  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
	  return -1;
	}
      old_path_str = (char *) malloc (old_path.len + 1);
      new_path_str = (char *) malloc (new_path.len + 1);
      memcpy (old_path_str, old_path.arr, old_path.len);
      old_path_str[old_path.len + 1] = '\0';
      memcpy (new_path_str, new_path.arr, new_path.len);
      old_path_str[new_path.len + 1] = '\0';
      if (stat (old_path_str, &fst))
	{
	  printf ("Missing image file %s\n", old_path_str);
	}
      else
	{
	  glob_t pglob;
	  char *glob_str = (char *) malloc (old_path.len + 3);
	  int i;

	  strcpy (glob_str, old_path_str);
	  strcat (glob_str, ".*");
	  max_size -= fst.st_size;

	  if (glob (glob_str, GLOB_ERR, NULL, &pglob))
	    {
	      char **fn = pglob.gl_pathv;
	      mvf = (struct move_files *) malloc (1 + pglob.gl_pathc);
	      mvf->old_path = old_path_str;
	      mvf->new_path = new_path_str;
	      tmvf = mvf;
	      tmvf++;
	      for (; fn; fn++, tmvf++, move_files_count++)
		{
		  char *new_glob_path =
		    (char *) malloc (new_path.len + strlen (*fn) -
				     old_path.len + 1);
		  strcpy (new_glob_path, old_path_str);
		  strcat (new_glob_path, *fn + old_path.len + 1);
		  printf ("%s->%s", *fn, new_glob_path);
		  stat (*fn, &fst);
		  max_size -= fst.st_size;
		  tmvf->old_path = strdup (*fn);
		  tmvf->new_path = new_glob_path;
		}
	    }
	  else
	    {
	      mvf = (struct move_files *) malloc (1);
	      mvf->old_path = old_path_str;
	      mvf->new_path = new_path_str;
	    }
	  free (glob_str);
	  globfree (&pglob);
	  printf ("globbing OK\n");

	  if (max_size < 0)
	    {
	      printf ("files to big, ending");
	      EXEC SQL CLOSE images_to_move;
	      EXEC SQL END;
	      return 0;
	    }

	  if (do_move)
	    {
	      char *new_dir = strdup (mvf->new_path);
	      new_dir = dirname (new_dir);
	      mkpath (new_dir, 0777);
	      free (new_dir);
	      EXEC SQL UPDATE images SET med_id =:img_new_med_id WHERE
		img_date =:img_date AND camera_name =:camera_name AND
		mount_name =:mount_name;
	    };

	  printf ("dir make, db changed\n");

	  for (i = 0, tmvf = mvf; i < move_files_count; i++, tmvf++)
	    {
	      printf ("%s->%s\n", tmvf->old_path, tmvf->new_path);
	      if (do_move)
		{
		  mv (tmvf->old_path, tmvf->new_path);
		};
	      free (tmvf->old_path);
	      free (tmvf->new_path);
	    }
	  free (mvf);
	}
    }
  printf ("last row sucessfully fetched.\n");

  EXEC SQL CLOSE images_to_move;
  EXEC SQL END;
  return 0;
}

int
main (int argc, char **argv)
{
  int c;
  char *epoch_name;
//  char cameras_names[500];
  long max_size = 0;
  int new_med = -1;
  int old_med = -1;

//  cameras_names[0] = 0;
  epoch_name = "002";

  while (1)
    {
      c = getopt (argc, argv, "hin:m:v");
      if (c == -1)
	break;
      switch (c)
	{
/*	case 'c':
	  strcat (camera_names, optarg);
	  strcat (camera_names, "|");
	  break; */
	case 'e':
	  if (epoch_name)
	    {
	      fprintf (stderr,
		       "Cannot work with more than three epoch names.\n");
	      exit (EXIT_FAILURE);
	    }
	  if (strlen (optarg) != 3)
	    {
	      fprintf (stderr,
		       "Epoch name should have exactly three places.\n");
	      exit (EXIT_FAILURE);
	    }
	  epoch_name = optarg;
	  break;
	case 'h':
	  printf ("Move images from one medias location to other.\n"
		  "Invocation\n"
		  "\t%s [options] <maximal_size_in_bytes>\n" "Options:\n"
//                "\t-c <camera_name>    move images only for camera with given name\n"
		  "\t-e <epoch_name>     name of epoch\n"
		  "\t-n <med_id>         new media id (must exists in database)\n"
		  "\t-m <med_id>         old media id (from where images will be moved)\n"
		  "\t-h                  prints that help\n"
		  "\t-i                  don't do any move, just print what will be done\n"
		  "  Size is given in bytes\n", argv[0]);
	  exit (EXIT_SUCCESS);
	case 'i':
	  do_move = 0;
	  break;
	case 'n':
	  new_med = atoi (optarg);
	  break;
	case 'm':
	  old_med = atoi (optarg);
	  break;
	case 'v':
	  verbose++;
	  break;
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind != argc - 1)
    {
      fprintf (stderr, " You must specifie new size as only parameter!\n");
      exit (EXIT_FAILURE);
    }
  max_size = atoi (argv[optind++]);
  if (epoch_name)
    printf ("Epoch name: %s\n", epoch_name);
  if (new_med == -1 || old_med == -1)
    {
      fprintf (stderr, "Medias id must be given!\n");
      exit (EXIT_FAILURE);
    }
  if (new_med == old_med)
    {
      fprintf (stderr, "Medias ids must be different!\n");
      exit (EXIT_FAILURE);
    }
  db_connect ();
  move_images (epoch_name, old_med, new_med, max_size);
  db_disconnect ();
  return 0;
}
