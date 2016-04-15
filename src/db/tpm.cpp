/* 
 * TPoint input builder.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include <string>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <libnova/libnova.h>
#include <wcstools/wcs.h>
#include <wcstools/fitsfile.h>

#include <ostream>
#include <iostream>

#include "cliapp.h"
#include "configuration.h"
#include "rts2format.h"
#include "rts2fits/image.h"


class TPM:public rts2core::CliApp
{
	public:
		TPM (int argc, char **argv);
		virtual ~ TPM (void);

		virtual int doProcessing ();

		virtual void help ();
		virtual void usage ();

	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
		virtual int init ();

	private:
		std::vector < std::string > filenames;
		int headline (rts2image::Image * image, std::ostream & _os);
		int printImage (rts2image::Image * image, WorldCoor * wcs, std::ostream & _os);

		struct ln_lnlat_posn obs;

		double ra_step;
		double ra_offset;
		double dec_step;
		double dec_offset;
		double x_ref;
		double y_ref;

		enum { TARGET, BEST, MOUNT } tarCorType;

		bool forceRawComputation;
		bool rawUseJ2k;
};

TPM::TPM (int in_argc, char **in_argv):rts2core::CliApp (in_argc, in_argv)
{
	tarCorType = MOUNT;
	ra_step = NAN;
	ra_offset = 0;
	dec_step = NAN;
	dec_offset = 0;
	x_ref = NAN;
	y_ref = NAN;

	forceRawComputation = false;
	rawUseJ2k = false;

	addOption ('p', NULL, 1, "specify reference pixel on image (by default taken from CRPIX), possible values are \"center\" or X:Y");
	addOption ('c', NULL, 1, "force using computation method to get raw mount RA/DEC coordinates from TEL and MNT_FLIP");
	addOption ('j', NULL, 1, "raw mount RA/DEC values (U_TEL and/or TEL) are stored in J2000");
	addOption ('r', NULL, 1, "step size for mnt_ax0; if specified, HA value is taken from mnt_ax0");
	addOption ('R', NULL, 1, "ra offset in raw counts");
	addOption ('d', NULL, 1, "step size for mnt_ax1; if specified, DEC value is taken from mnt_ax1");
	addOption ('D', NULL, 1, "dec offset in raw counts");
}

TPM::~TPM (void)
{
	filenames.clear ();
}

int TPM::processOption (int in_opt)
{
	std::vector <std::string> params;

	switch (in_opt)
	{
		case 'p':
			if (strcmp(optarg, "center") == 0)
			{
				x_ref = -1.0;
				y_ref = -1.0;
			} 
			else
			{
				params = SplitStr (optarg, ":");
				if (params.size () != 2)
				{
					std::cerr << "cannot parse -p parameter " << optarg << std::endl;
					return -1;
				}
				x_ref = atof (params[0].c_str ());
				y_ref = atof (params[1].c_str ());
			}
			break;
		case 'c':
			forceRawComputation = true;
			break;
		case 'j':
			rawUseJ2k = true;
			break;
		case 'r':
			ra_step = atof (optarg);
			break;
		case 'R':
			ra_offset = atof (optarg);
			break;
		case 'd':
			dec_step = atof (optarg);
			break;
		case 'D':
			dec_offset = atof (optarg);
			break;
		default:
			return rts2core::CliApp::processOption (in_opt);
	}
	return 0;
}

int TPM::processArgs (const char *arg)
{
	filenames.push_back (arg);
	return 0;
}

int TPM::init ()
{
	int ret;
	ret = rts2core::CliApp::init ();
	if (ret)
		return ret;
	if (filenames.empty ())
	{
		std::cerr << "Need to have at least one filename!" << std::endl;
		help ();
		return -1;
	}
	return 0;
}

int TPM::doProcessing ()
{
	bool firstLine = false;
	char filename[100];
	char *header;
	int lhead, nbfits;
	struct WorldCoor *wcs;

	for (std::vector < std::string >::iterator iter = filenames.begin (); iter != filenames.end (); iter++)
	{
		strcpy (filename, const_cast<char*>(iter->c_str ()));
		if (isfits(filename))
		{
			header = fitsrhead (filename, &lhead, &nbfits);
			if (header == NULL)
				wcs = NULL;
			else
			{
				wcs = wcsinit (header);
				free (header);
			}

			if (!nowcs (wcs))
			{
				rts2image::Image *image = new rts2image::Image ();
				image->openFile (filename, true, true);
				if (!firstLine)
				{
					headline (image, std::cout);
					firstLine = true;
				}
				printImage (image, wcs, std::cout);
				delete image;
				wcsfree (wcs);
			}
			else
				std::cerr << filename << ": no wcs found..." << std::endl;
		}
		else
			std::cerr << filename << ": is not fits format..." << std::endl;
	}
	std::cout << "END" << std::endl;
	return 0;
}

void TPM::help ()
{
	std::cout << "Process list of images with astrometry, and creates file which you can feed to TPoint. "
		"Without any option, prouduce file with J2000 mean coordinates. "
		"The output produced should be sufficient to use it in TPOINT to get the pointing model."
		<< std::endl;
	rts2core::CliApp::help ();
}

void TPM::usage ()
{
	std::cout << "\t" << getAppName () << " *.fits > model.in" << std::endl;
}

int TPM::headline (rts2image::Image * image, std::ostream & _os)
{
	obs.lat = rts2core::Configuration::instance ()->getObserver ()->lat;
	obs.lng = rts2core::Configuration::instance ()->getObserver ()->lng;
	// try to get latitude from image
	image->getValue ("LATITUDE", obs.lat);
	image->getValue ("LONGITUD", obs.lng);
	// standart header
								 // we are observing on equatorial mount
	_os << "RTS2 model from astrometry" << std::endl << ":EQUAT" << std::endl;
	if (rawUseJ2k)
	{
		_os << ":J2000" << std::endl;
	}
	_os << ":NODA" << std::endl; // don't know
	_os << " " << LibnovaDeg90 (obs.lat)
		<< " " << image->getYearString ()
		<< " " << image->getMonthString ()
		<< " " << image->getDayString () << " 20 1000 60" << std::endl;
	return 0;
}

int TPM::printImage (rts2image::Image * image, WorldCoor * wcs, std::ostream & _os)
{
	LibnovaRaDec actual;
	LibnovaRaDec target;
	time_t ct;
	double JD;
	double mean_sidereal;
	float expo;
	double aux0 = NAN;
	double aux1 = NAN;
	int num_pixels;
	double x_pos, y_pos;

	if (isnan (x_ref) || isnan (y_ref))
		image->getCoordAstrometry (actual);
	else
	{
		if (x_ref < 0.0 || y_ref < 0.0)	// use center of image
		{
			image->getValue ("NAXIS1", num_pixels, true);
			x_ref = num_pixels / 2.0;
			image->getValue ("NAXIS2", num_pixels, true);
			y_ref = num_pixels / 2.0;
		}
		pix2wcs (wcs, x_ref, y_ref, &x_pos, &y_pos);
		actual.setRa (x_pos);
		actual.setDec (y_pos);
	}

	if (forceRawComputation)
		image->getCoordMountRawComputed (target);
	else
		image->getCoordMountRawBest (target);

	image->getValue ("CTIME", ct);
	image->getValue ("EXPOSURE", expo);

	aux0 = -2;
	image->getValue ("MNT_AX0", aux1, false);

	aux1 = -2;
	image->getValue ("MNT_AX1", aux1, false);

	ct = (time_t) (ct + expo / 2.);

	JD = ln_get_julian_from_timet (&ct);
	mean_sidereal = ln_range_degrees (15. * ln_get_apparent_sidereal_time (JD) + obs.lng);

	if (!isnan (ra_step))
	{
		image->getValue ("MNT_AX0", aux0, true);
		target.setRa (ln_range_degrees (mean_sidereal - ((aux0 - ra_offset) / ra_step)));
	}
	if (!isnan (dec_step))
	{
		image->getValue ("MNT_AX1", aux1, true);
		target.setDec ((aux1 - dec_offset) / dec_step);
	}

	LibnovaHaM lst (mean_sidereal);

	_os << spaceDegSep << actual << " 0 0 2000.0 ";
	_os << spaceDegSep << target << " " << lst << " " << aux0 << " " << aux1 << std::endl;
	return 0;
}

int main (int argc, char **argv)

{
	TPM app = TPM (argc, argv);
	return app.run ();
}
