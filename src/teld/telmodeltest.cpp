/* 
 * Dry run for telescope model.
 * Copyright (C) 2006-2008 Petr Kubanek <petr@kubanek.net>
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

#include "telmodel.h"
#include "libnova_cpp.h"
#include "rts2format.h"
#include "cliapp.h"
#include "rts2fits/imagedb.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace rts2telmodel;

namespace rts2teld
{

class ModelTest:public Telescope
{
	public:
		ModelTest ():Telescope (0, NULL)
		{
			createConstValue (telLongitude, "LONGITUD", "telescope longtitude");
			createConstValue (telLatitude, "LATITUDE", "telescope latitude");
			createConstValue (telAltitude, "ALTITUDE", "telescope altitude");

			telLongitude->setValueDouble (rts2core::Configuration::instance ()->getObserver ()->lng);
			telLatitude->setValueDouble (rts2core::Configuration::instance ()->getObserver ()->lat);
		}

		void setObserverLat (double in_lat) { telLatitude->setValueDouble (in_lat); }

	protected:
		int startResync () { return 0; }
		int isMoving () { return -2; }
		int stopMove () { return 0; }
		int startPark () { return 0; }
		int endPark () { return 0; }
};

class TelModelTest:public rts2core::CliApp
{
	private:
		char *modelFile;
		Model *model;
		std::vector < std::string > runFiles;
		ModelTest *telescope;
		int errors;
		bool verbose;
		// if input are images
		bool image;
		bool rpoint;

		void test (double ra, double dec);
		void runOnFile (std::string filename, std::ostream & os);
		void runOnFitsFile (std::string filename, std::ostream & os);
		void runOnDatFile (std::string filename, std::ostream & os);
	protected:
		virtual void usage ();
		
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual int doProcessing ();

	public:
		TelModelTest (int in_argc, char **in_argv);
		virtual ~ TelModelTest (void);

		virtual int init ();
};

};

using namespace rts2teld;

TelModelTest::TelModelTest (int in_argc, char **in_argv):rts2core::CliApp (in_argc, in_argv)
{
	rts2core::Configuration::instance ();
	modelFile = NULL;
	model = NULL;
	telescope = NULL;
	errors = 0;
	image = false;
	rpoint = false;
	verbose = false;
	addOption ('m', NULL, 1, "Model file to use");
	addOption ('e', NULL, 0, "Print errors. Use two e to print errors in RA and DEC.");
	addOption ('N', NULL, 0, "Print numbers, do not pretty print.");
	addOption ('v', NULL, 0, "Report model progress");
	addOption ('i', NULL, 0, "Print model for given images");
	addOption ('r', NULL, 0, "Print random RA DEC, handy for telescope pointing tests");
}

TelModelTest::~TelModelTest (void)
{
	delete telescope;
	runFiles.clear ();
}

void TelModelTest::usage ()
{
	std::cout << "To run on TPOINT input named model_test.dat, with model file in /etc/rts2/model:" << std::endl 
		<< "\t" << getAppName () << " -m /etc/rts2/model -e -v model_test.dat" << std::endl
	<< "To generate random pointings" << std::endl
		<< "\t" << getAppName () << " -r" << std::endl;
} 

int TelModelTest::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'm':
			modelFile = optarg;
			break;
		case 'e':
			errors++;
			break;
		case 'N':
			std::cout << pureNumbers;
			break;
		case 'v':
			verbose = true;
			break;
		case 'i':
			image = true;
			break;
		case 'r':
			rpoint = true;
			break;
		default:
			return rts2core::App::processOption (in_opt);
	}
	return 0;
}

int TelModelTest::processArgs (const char *in_arg)
{
	runFiles.push_back (in_arg);
	return 0;
}

int TelModelTest::init ()
{
	int ret;
	ret = rts2core::App::init ();
	if (ret)
		return ret;

	if (!rpoint && runFiles.empty ())
	{
		help ();
		return -1;
	}

	if (rpoint)
		return 0;

	telescope = new ModelTest ();
	telescope->setCorrections (false, false, false);

	model = new Model (telescope, modelFile);
	ret = model->load ();

	return ret;
}

void TelModelTest::test (double ra, double dec)
{
	struct ln_equ_posn pos;
	pos.ra = ra;
	pos.dec = dec;
	model->apply (&pos);
	std::cout << "RA: " << ra << " DEC: " << dec << " -> " << pos.ra << " " << pos.dec << std::endl;
}

void TelModelTest::runOnFile (std::string filename, std::ostream & os)
{
	if (image)
		runOnFitsFile (filename, os);
	else
		runOnDatFile (filename, os);
}

void TelModelTest::runOnFitsFile (std::string filename, std::ostream & os)
{
	// load image data, open them read-only
	rts2image::ImageDb img;
	img.openFile (filename.c_str (), true, true);
	struct ln_equ_posn posObj;
	struct ln_equ_posn posTar;
	struct ln_equ_posn posImg;
	LibnovaRaDec posMount;

	img.getCoordObject (posObj);
	img.getCoordTarget (posTar);
	img.getCoordAstrometry (posImg);
	img.getCoordMount (posMount);

	LibnovaRaDec pObj (&posObj);
	LibnovaRaDec pTar (&posTar);
	os << "Object: " << pObj << std::endl
	  << "Target: " << pTar << std::endl;

	double lst = img.getExposureLST ();
	posTar.ra = ln_range_degrees (lst - posTar.ra);
	if (verbose)
		model->reverseVerbose (&posTar);
	else
		model->reverse (&posTar);
	posTar.ra = ln_range_degrees (lst - posTar.ra);
	LibnovaRaDec pTar2 (&posTar);
	LibnovaRaDec pImg (&posImg);

	LibnovaDegDist modTarRa (posTar.ra - pTar.getRa ());
	LibnovaDegDist modTarDec (posTar.dec - pTar.getDec ());

	LibnovaDegDist objImgRa (pImg.getRa () - pObj.getRa ());
	LibnovaDegDist objImgDec (pImg.getDec () - pObj.getDec ());

	LibnovaDegDist tarImgRa (pImg.getRa () - pTar.getRa ());
	LibnovaDegDist tarImgDec (pImg.getDec () - pTar.getDec ());

	os << "Model:  " << pTar2 << " " << modTarRa << " " << modTarDec << std::endl
		<< "Mount:  " << posMount << std::endl
		<< "Image:  " << pImg << " " << objImgRa << " " << objImgDec 
		<< " " << tarImgRa << " " << tarImgDec << std::endl;
}

void TelModelTest::runOnDatFile (std::string filename, std::ostream & os)
{
	char caption[81];
	double temp;
	double press;
	std::ifstream is (filename.c_str ());
	char firstChar;
	bool latLine = false;

	double rms = 0;
	int rms_n = 0;

	// date when data were acquired
	struct ln_date date;

	// julian date (of observations start; as precession will not change a much in few hours spanning data acqusition time,
	// we don't need exact date
	double JD = rts2_nan ("f");

	is.getline (caption, 80);
	os << caption << spaceDegSep << std::endl;
	while (!is.eof ())
	{
		// ignore
		firstChar = is.peek ();
		if (firstChar == ':')
		{
			is.getline (caption, 80);
			os << caption << std::endl;
		}
		else if (firstChar == 'E')
		{
			std::string nd;
			is >> nd;
			if (nd == "END")
			{
				if (errors)
				{
					rms = sqrt (rms / (double) rms_n);
					double rms_e = rms - model->getRMS ();
					os << "RMS: " << (rms * 3600.0) << " (" << LibnovaDegDist (rms)
						<< ") n: " << rms_n << " RMS error: " << (rms_e * 3600.0)
						<< " (" << ((fabs (rms) / model->getRMS ()) * 100) << "%)" << std::endl;
				}
				os << "END" << std::endl;
				return;
			}
			logStream (MESSAGE_ERROR) << "Invalid end? " << nd << sendLog;
		}
		// first line contains lat
		else if (!latLine)
		{
			struct ln_dms lat;
			std::string line;
			std::getline (is, line);
			std::istringstream iss (line);
			date.hours = 0;
			date.minutes = 0;
			date.seconds = 0;
			// get sign
			if (isblank (firstChar))
				iss >> firstChar;

			iss >> lat.degrees >> lat.minutes >> lat.seconds;

			iss >> date.years >> date.months >> date.days;
			if (iss.fail ())
			{
				date.years = 2000;
				date.months = 1;
				date.days = 1;
				temp = rts2_nan ("f");
				press = rts2_nan ("f");
			}
			else
			{
				JD = ln_get_julian_day (&date);
				iss >> temp >> press;
				if (iss.fail ())
				{
					temp = rts2_nan ("f");
					press = rts2_nan ("f");
					telescope->setCorrections (true, true, true);
				}
				else
				{
					telescope->setCorrections (true, true, false);
				}
			}
			if (firstChar == '-')
				lat.neg = 1;
			else
				lat.neg = 0;
			telescope->setObserverLat (ln_dms_to_deg (&lat));
			latLine = true;
			os << " " << LibnovaDeg90 (ln_dms_to_deg (&lat)) << " "
				<< date.years << " " << date.months << " " << date.days;
			if (!isnan (temp))
			{
				os << " " << temp;
				if (!isnan (press))
					os << " " << press;
			}
			os << std::endl;
		}
		// data lines
		else
		{
			double aux1;
			double aux2;
			int m, d;
			double epoch;

			if (firstChar == '!')
				firstChar = is.get ();

			LibnovaRaDec _in;
			LibnovaRaDec _out;
			LibnovaHaM lst;

			is >> _in >> m >> d >> epoch >> _out >> lst >> aux1 >> aux2;
			is.ignore (2000, is.widen ('\n'));
			if (is.fail ())
			{
				logStream (MESSAGE_ERROR) << "Failed during file read" << sendLog;
				return;
			}
			// calculate model position, output it,..
			struct ln_equ_posn pos;
			_out.getPos (&pos);

			pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
			if (verbose)
				model->applyVerbose (&pos);
			else
				model->apply (&pos);
			// print it out
			pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
			std::cout.precision (1);
			LibnovaRaDec _out_in (&pos);

			if (errors)
			{
				struct ln_equ_posn pos_in, pos_out;
				_in.getPos (&pos_in);
				_out_in.getPos (&pos_out);

				// normalize dec..
				if (fabs (pos_out.dec) > 90)
				{
					pos_out.ra = ln_range_degrees (pos_out.ra + 180.0);
					pos_out.dec = (pos_out.dec > 0 ? 180 : -180) - pos_out.dec;
				}

				if (!isnan (JD))
					telescope->applyCorrections (&pos_in, JD);

				double err = ln_get_angular_separation (&pos_in, &pos_out);

				// do some statistics
				rms += err * err;
				rms_n++;

				std::cout << LibnovaDegDist (err) << " ";

				if (errors > 1)
				{
					double rd, dd;
					rd = pos_in.ra - pos_out.ra;
					dd = pos_in.dec - pos_out.dec;

					std::cout << LibnovaDegArcMin (rd) << " " << LibnovaDegArcMin (dd);
				}
			}

			std::cout << "  " << _in << " " << m << " " << d << " " << epoch << "   " << _out_in << "   " << lst << " ";
			std::cout.precision (6);
			std::cout << aux1 << " " << aux2 << std::endl;
		}
	}
}

int TelModelTest::doProcessing ()
{
	if (rpoint)
	{
		srandom (time (NULL));
		struct ln_hrz_posn hrz;
		hrz.alt = 90 - 85 * (((double) random ()) / RAND_MAX);
		hrz.az = 360 * (((double) random ()) / RAND_MAX);
		struct ln_equ_posn equ;
		ln_get_equ_from_hrz (&hrz, rts2core::Configuration::instance ()->getObserver (), ln_get_julian_from_sys (), &equ);
		std::cout << equ.ra << " " << equ.dec << std::endl;
		return 0;
	}
	if (!runFiles.empty ())
	{
		for (std::vector < std::string >::iterator iter = runFiles.begin (); iter != runFiles.end (); iter++)
			runOnFile ((*iter), std::cout);
	}
	else
	{
		// some generic tests
		test (10, 20);
		test (30, 50);
	}
	return 0;
}

int main (int argc, char **argv)
{
	TelModelTest app (argc, argv);
	return app.run ();
}
