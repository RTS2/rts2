/*
 * Utility to move images to backup location.
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

#include "utilsfunc.h"
#include "rts2db/appdb.h"
#include "rts2db/devicedb.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <libgen.h>
#include <iostream>

/**
 * Application class to move images from one media to the other.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2BckImageApp: public rts2db::AppDb
{
	public:
		Rts2BckImageApp (int argc, char **argv);

	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual int init ();

		virtual void usage ();

		virtual int doProcessing ();

	private:
		int verbose;
		int do_move;
		double medium_size;
		const char *epoch_name;

		int new_med;
		int old_med;
};

struct move_files
{
	char *old_path;
	char *new_path;
};

int Rts2BckImageApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		/*	case 'c':
			strcat (camera_names, optarg);
			strcat (camera_names, "|");
			break; */
		case 'e':
			if (epoch_name)
			{
				fprintf (stderr, "Cannot work with more than one epoch names.\n");
				exit (EXIT_FAILURE);
			}
			if (strlen (optarg) != 3)
			{
				fprintf (stderr, "Epoch name should have exactly three places.\n");
				exit (EXIT_FAILURE);
			}
			epoch_name = optarg;
			break;
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
		default:
			return rts2db::AppDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2BckImageApp::processArgs (const char *arg)
{
	int vali = strlen (arg) - 1;
	char c = arg[vali];
	char *cs = new char[vali] + 2;
	strcpy (cs, arg);
	if (isalpha (arg[vali]))
	{
		cs[vali] = 0;
		medium_size = atof (cs);
		switch (c)
		{
			case 'G':
			case 'g':
				// gb = 2 ^ 32 b
				medium_size *= 1024 * 1024 * 1024;
				break;
			case 'M':
			case 'm':
				medium_size *= 1024 * 1024;
				break;
			case 'K':
			case 'k':
				medium_size *= 1024;
				break;
			default:
				fprintf (stderr, "Unknow size value '%c'.\n", c);
				exit (EXIT_FAILURE);
		}
	}
	else
	{
		medium_size = atof (cs);
	}
	delete[] cs;
	return 0;
}

int Rts2BckImageApp::init ()
{
	int ret;
	ret = rts2db::AppDb::init ();
	if (ret)
		return ret;

	if (!epoch_name)
		epoch_name = "002";
	printf ("Epoch name: %s\n", epoch_name);
	printf ("Size: %.0f bytes\n", medium_size);
	if (new_med == -1 || old_med == -1)
	{
		fprintf (stderr, "Medias id must be given!\n");
		return -1;
	}
	if (new_med == old_med)
	{
		fprintf (stderr, "Medias ids must be different!\n");
		return -1;
	}
	return 0;
}

void Rts2BckImageApp::usage ()
{
	std::cout << "Move images from one medias location to other." << std::endl
		<< "   Invocation:" << std::endl
		<< getAppName () << " [options] <maximal_size_in_bytes>" << std::endl
		<< "  Size is given in bytes as n, MB as nM or GB as nG." << std::endl;
}

int Rts2BckImageApp::doProcessing ()
{
	if (rts2db::checkDbConnection ())
		return -1;

	double size_count = 0;
	int img_move_count = 0;
	int file_move_count = 0;
	EXEC SQL BEGIN DECLARE SECTION;
		VARCHAR old_path[200];
		VARCHAR new_path[200];
		VARCHAR camera_name[8];
		VARCHAR mount_name[8];
		const char *img_epoch_name = epoch_name;
		long int img_date;
		int img_new_med_id = new_med;
		int img_id;
		int old_med_id = old_med;
	EXEC SQL END DECLARE SECTION;
	struct stat fst;
	struct move_files *mvf, *tmvf;
	char *new_path_str, *old_path_str;

	EXEC SQL CONNECT TO stars;

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
			mount_name, EXTRACT (EPOCH FROM img_date), img_id FROM images,
			observations WHERE images.obs_id =
			observations.obs_id AND epoch_id =:img_epoch_name AND med_id =:old_med_id
			ORDER BY img_date ASC;

	EXEC SQL OPEN images_to_move;

	printf ("Fetching rows\n");

	while (!sqlca.sqlcode)
	{
		double tmp_count = 0;
		int move_files_count = 1;
		EXEC SQL FETCH next FROM images_to_move
			INTO:old_path,:new_path,:camera_name,:mount_name,:img_date,:img_id;
		if (sqlca.sqlcode < 0)
		{
			printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
			return -1;
		}
		old_path_str = (char *) malloc (old_path.len + 1);
		new_path_str = (char *) malloc (new_path.len + 1);
		memcpy (old_path_str, old_path.arr, old_path.len);
		old_path_str[old_path.len] = 0;
		memcpy (new_path_str, new_path.arr, new_path.len);
		new_path_str[new_path.len] = 0;
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
			medium_size -= fst.st_size;
			tmp_count += fst.st_size;
			if (!glob (glob_str, GLOB_ERR, NULL, &pglob))
			{
				char **fn = pglob.gl_pathv;
				mvf =
					(struct move_files *) malloc (sizeof (struct move_files) *
					(pglob.gl_pathc + 1));
				mvf->old_path = old_path_str;
				mvf->new_path = new_path_str;
				tmvf = mvf;
				tmvf++;
				for (; *fn; fn++, tmvf++, move_files_count++)
				{
					char *new_glob_path =
						(char *) malloc (new_path.len + strlen (*fn) -
						old_path.len + 1);
					strcpy (new_glob_path, new_path_str);
					strcat (new_glob_path, *fn + old_path.len);
					if (stat (*fn, &fst))
					{
						perror ("stat");
						exit (EXIT_FAILURE);
					}
					medium_size -= fst.st_size;
					tmp_count += fst.st_size;
					tmvf->old_path = strdup (*fn);
					tmvf->new_path = new_glob_path;
				}
			}
			else
			{
				mvf = (struct move_files *) malloc (sizeof (struct move_files));
				mvf->old_path = old_path_str;
				mvf->new_path = new_path_str;
			}
			free (glob_str);
			globfree (&pglob);

			if (medium_size < 0)
			{
				printf
					("Finished after moving %i images (%i files, %.0f bytes), size exceeded.\n",
					img_move_count, file_move_count, size_count);
				EXEC SQL CLOSE images_to_move;
				EXEC SQL END;
				return 0;
			}

			size_count += tmp_count;
			if (do_move)
			{
				char *new_dir = strdup (mvf->new_path);
				new_dir = dirname (new_dir);
				if (mkpath (new_dir, 0777))
				{
					fprintf (stderr, "While creating path %s:", new_dir);
					perror ("");
					exit (EXIT_FAILURE);
				}
				free (new_dir);
			}
			img_move_count++;
			for (i = 0, tmvf = mvf; i < move_files_count; i++, tmvf++)
			{
				printf ("%s -> %s", tmvf->old_path, tmvf->new_path);
				if (do_move)
				{

					if (rename (tmvf->old_path, tmvf->new_path))
					{
						printf ("..failed\n");
						perror ("mv failed");
						exit (EXIT_FAILURE);
					}
					else
						printf ("..ok\n");
				}
				else
				{
					printf ("..not done\n");
				}
				file_move_count++;
				free (tmvf->old_path);
				free (tmvf->new_path);
			}
			if (do_move)
			{
				EXEC SQL UPDATE images SET med_id =:img_new_med_id WHERE
						img_id =:img_id;
			}
			free (mvf);
		}
	}
	printf
		("No more rows. All images were backed-up, please check media %i dirs before removing them.\n%i images (%i files) were moved (%.0f bytes of data)\n",
		old_med_id, img_move_count, file_move_count, size_count);

	EXEC SQL CLOSE images_to_move;
	EXEC SQL END;
	EXEC SQL DISCONNECT;
	return 0;
}

Rts2BckImageApp::Rts2BckImageApp (int argc, char **argv):rts2db::AppDb (argc, argv)
{
	verbose = 0;
	do_move = 1;
	medium_size = 0;
	epoch_name = NULL;

	new_med = -1;
	old_med = -1;

	addOption ('e', NULL, 1, "name of epoch from which the images will be backed-up");
	addOption ('n', NULL, 1, "new media id (must exists in database)");
	addOption ('m', NULL, 1, "old media id (from where images will be moved)");
	addOption ('i', NULL, 1, "don't do any move, just print what will be done");
}

int main (int argc, char **argv)
{
	Rts2BckImageApp app = Rts2BckImageApp (argc, argv);
	return app.run ();
	return 0;
}
