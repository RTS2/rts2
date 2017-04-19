#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "configuration.h"
#include <fitsio.h>

#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

rts2core::Configuration *config;

int verbose = 0;
int do_unlink = 1;
double max, min;
int recursive = 0;

void
check_unlink (char *filename)
{
	if (do_unlink == 0)
	{
		printf ("Will remove %s\n", filename);
		return;
	}
	if (do_unlink == -1)
	{
		int y;
		printf ("Remove %s (y/n)?", filename);
		y = getchar ();
		if (y != 'y' || y != 'Y')
			return;
	}
	if (verbose)
		printf ("Remove %s\n", filename);
	if (unlink (filename) == -1)
		perror ("unlinking");
	return;
}


void process_file (char *filename);

void
process_dir (char *dirname)
{
	DIR *dir;
	struct dirent *de;
	struct stat st;
	if ((dir = opendir (dirname)) == NULL)
	{
		perror ("opendir");
		return;
	}
	if (chdir (dirname) == -1)
	{
		perror ("chdir");
		return;
	}
	while ((de = readdir (dir)) != NULL)
	{
		if (stat (de->d_name, &st) == -1)
		{
			perror ("stat");
			continue;
		}
		if (S_ISDIR (st.st_mode) && !strcmp (de->d_name, ".")
			&& !strcmp (de->d_name, ".."))
			process_dir (de->d_name);
		else if (S_ISREG (st.st_mode) || S_ISLNK (st.st_mode))
			process_file (de->d_name);
	}
	if (chdir ("..") == -1)
		perror ("chdir ..");
}


void
process_file (char *filename)
{
	fitsfile *fptr;
	int status, nfound, anynull;
	double median;
	long naxes[4], fpixel, nbuffer, npixels, npix, ii;
	char camera_name[80];

	#define BUFFSIZE  1000
	float nullval, buffer[BUFFSIZE];

	#define printerror(sta)   fits_report_error (stderr, status)
	status = 0;
	if (fits_open_file (&fptr, filename, READONLY, &status))
	{
		printerror (status);
		return;
	}
	if (fits_read_keys_lng (fptr, (char *) "NAXIS", 1, 2, naxes, &nfound, &status))
		goto err;
	if (nfound < 1)
	{
		if (verbose)
			printf ("No image found in %s\n", filename);
		fits_close_file (fptr, &status);
		check_unlink (filename);
		return;
	}
	nfound--;
	npixels = naxes[nfound];
	for (; nfound > 0; nfound--)
		npixels *= naxes[nfound];
	npix = npixels;
	if (fits_read_key_str (fptr, (char *) "CAM_NAME", camera_name, NULL, &status))
	{
		if (verbose)
			printf ("No CAM_NAME in %s, default will be used\n", filename);
		if (std::isnan (min))
		{
			if (config->getDouble ("flatprocess", "min", min))
				min = -INFINITY;
		}
		if (std::isnan (max))
		{
			if (config->getDouble ("flatprocess", "max", max))
				max = INFINITY;
		}
		status = 0;
	}
	else
	{
		if (std::isnan (min))
		{
			if (config->getDouble (camera_name, "flatmin", min))
				min = -INFINITY;
		}
		if (std::isnan (max))
		{
			if (config->getDouble (camera_name, "flatmax", max))
				max = INFINITY;
		}
	}
	if (verbose > 1)
		printf ("%s: min %f, max %f, npix: %li\n", filename, min, max, npixels);
	fpixel = 1;
	median = 0;
	while (npixels > 0)
	{
		if (npixels > BUFFSIZE)
			nbuffer = BUFFSIZE;
		else
			nbuffer = npixels;
		if (fits_read_img
			(fptr, TFLOAT, fpixel, nbuffer, &nullval, buffer, &anynull,
			&status))
			goto err;
		for (ii = 0; ii < nbuffer; ii++)
			median += buffer[ii];
		npixels -= nbuffer;
		fpixel += nbuffer;
	}
	median = median / npix;
	if (verbose)
		printf ("%s %f\n", filename, median);
	if (fits_close_file (fptr, &status))
		printerror (status);
	if (median < min || median > max)
		check_unlink (filename);
	return;
	err:
	printerror (status);
	if (fits_close_file (fptr, &status))
		printerror (status);
	return;
}


int
main (int argc, char **argv)
{
	int c;
	struct stat st;

	max = NAN; 
	min = NAN;

	while (1)
	{
		c = getopt (argc, argv, "hvinm:M:r");
		if (c == -1)
			break;
		switch (c)
		{
			case 'i':
				if (do_unlink != 1)
				{
					fprintf (stderr,
						"Cannot specifi interactive with not proceed.\n");
					exit (EXIT_FAILURE);
				}
				do_unlink = -1;
				break;
			case 'n':
				if (do_unlink != 1)
				{
					fprintf (stderr,
						"Cannot specifi not proceed with interactive.\n");
					exit (EXIT_FAILURE);
				}
				do_unlink = 0;
				break;
			case 'm':
				min = atof (optarg);
				break;
			case 'M':
				max = atof (optarg);
				break;
			case 'r':
				recursive = 1;
				break;
			case 'h':
				printf ("Remove bad flats (with values more common for \n"
					" darks or for overexposed images)\n"
					"Invocation:\n"
					"\t%s [options] <flat_names>\n"
					"Options:\n"
					"\t-i		interactive\n"
					"\t-n		don't unlink bad flats, just print their values\n"
					"\t-m		min value of median for good flat (overwrites config value)\n"
					"\t-M		max value of median for good flat (overwrites config value)\n"
					"\t-h		that help\n"
					"\t-r		recursive (check directories for *.fit*)\n"
					"\t-v		verbose (increase verbosity)\n"
					" Part of rts2 package.\n", argv[0]);
				exit (EXIT_SUCCESS);
			case 'v':
				verbose++;
				break;
			case '?':
				break;
			default:
				fprintf (stderr, "?? getopt returned unknow character %o ??\n", c);
		}
	}
	if (optind == argc)
	{
		fprintf (stderr, "No file specified!\n");
		exit (EXIT_FAILURE);
	}

	config = rts2core::Configuration::instance ();
	config->loadFile ();

	for (argv = &argv[optind++]; *argv; argv++)
	{
		if (stat (*argv, &st) == -1)
		{
			perror ("stat");
			exit (EXIT_FAILURE);
		}
		if (S_ISDIR (st.st_mode))
		{
			if (recursive)
				process_dir (*argv);
			else if (verbose)
				printf ("Directory %s ignored, not in recursive mode\n", *argv);
		}
		else if (S_ISREG (st.st_mode) || S_ISLNK (st.st_mode))
			process_file (*argv);
		else
			fprintf (stderr, "Not a file nor directory");
	}
	return 0;
}
