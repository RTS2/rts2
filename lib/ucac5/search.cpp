/* 
 * UCAC5 indexing application
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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

#include "app.h"
#include "radecparser.h"
#include "UCAC5Record.hpp"
#include "UCAC5Idx.hpp"
#include "UCAC5Bands.hpp"

#include <libnova_cpp.h>

#include <errno.h>
#include <string>
#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

class UCAC5Search:public rts2core::App
{
	public:
		UCAC5Search (int argc, char **argv);

		virtual int run();
	
	protected:
		virtual int processOption (int opt);
		virtual int processArgs (const char *arg);
	
	private:
		std::string radec;
		double ra;
		double dec;
		double minRad;
		double maxRad;
		int argCount;
		int verbose;
		const char *base;
};

UCAC5Search::UCAC5Search (int argc, char **argv):App (argc, argv), radec(""), ra(NAN), dec(NAN), minRad(NAN), maxRad(NAN), argCount(0), verbose(0), base("~/ucac5")
{
	addOption('v', NULL, 0, "increases verbosity");
	addOption('b', NULL, 1, "UCAC5 base path");
}

int UCAC5Search::run()
{
	int ret = init();
	if (ret)
		return ret;
	ret = chdir(base);
	if (ret)
	{
		std::cerr << "cannot change directory to " << base << ":" << strerror(errno) << std::endl;
		return -1;
	}

	std::cout << "# searching " << LibnovaRaDec(ra, dec) << " <" << minRad << "," << maxRad << ">" << std::endl;

	UCAC5Bands bands;
	ret = bands.openBand("u5index.unf");
	if (ret)
	{
		std::cerr << "cannot open band index file " << base << "/u5index.unf" << std::endl;
		return -1;
	}

	double ra_r = D2R * ra, dec_r = D2R * dec, min_r = AS2R * minRad, max_r = AS2R * maxRad;

	UCAC5Idx index;
	uint16_t dec_b = 0, ra_b = 0;
	uint32_t ra_start = 0;
	int32_t len;
	while ((bands.nextBand(ra_r, dec_r, max_r, dec_b, ra_b, ra_start, len)) == 0)
	{
		if (verbose)
			std::cout << "# band " << dec_b << " RA " << ra_b << " (" << ra_start << ".." << len << ")" << std::endl;
	}

	return 0;
}

int UCAC5Search::processOption (int opt)
{
	switch (opt)
	{
		case 'v':
			verbose++;
			break;
		case 'b':
			base = optarg;
			break;
		default:
			return App::processOption(opt);
	}
	return 0;
}

int UCAC5Search::processArgs (const char *arg)
{
	switch (argCount)
	{
		case 0:
		case 1:
			radec += std::string(" ") + arg;
			if (argCount == 1)
				parseRaDec(radec.c_str(), ra, dec);
			break;
		case 2:
			minRad = atof(arg);
			break;
		case 3:
			maxRad = atof(arg);
			break;
	}
	argCount++;

	return 0;
}

int main (int argc, char **argv)
{
	UCAC5Search app(argc, argv);
	return app.run();
}
