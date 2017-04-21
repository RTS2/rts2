/* 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

Copyright 2005 Petr Kubanek	*/

#include "rts2db/appdb.h"

#include <iostream>
#include <math.h>
#include <libnova/libnova.h>

class Rts2AirmasScale: public rts2db::AppDb
{
	public:
		Rts2AirmasScale (int argc, char **argv);

		virtual int doProcessing ();

	private:
		enum {printAction, setAirmass}
		action;
		double steps;
		double max;

		// action functions
		int doSetAirmass ();
		int doPrintAction ();

		virtual void help ();
		virtual void usage ();

		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual int init ();
};

Rts2AirmasScale::Rts2AirmasScale (int in_argc, char **in_argv): rts2db::AppDb (in_argc, in_argv)
{
	steps = NAN;
	max = NAN;

	addOption ('s', NULL, 0, "set airmass scales. Two args are scale and max airmass");
}

void Rts2AirmasScale::help ()
{
	std::cout << "Without any option print current airmass-scale table" << std::endl;
	rts2db::AppDb::help ();
}

void Rts2AirmasScale::usage ()
{
	std::cout
		<< "Set airmass table from 0 to 2.5 with step size 0.2" << std::endl
		<< "\t" << getAppName () << " -s 0.2 2.5" << std::endl;
}

int Rts2AirmasScale::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			action = setAirmass;
			break;
		default:
			return rts2db::AppDb::processOption (in_opt);
	}
	return 0;
}

int Rts2AirmasScale::processArgs (const char *arg)
{
	// we expected steps-max
	if (std::isnan (steps))
	{
		steps = atof (arg);
		return steps == 0 ? -1 : 0;
	}
	else if (std::isnan (max))
	{
		max = atof (arg);
		return max == 0 ? -1 : 0;
	}
	return -1;
}

int Rts2AirmasScale::init ()
{
	int ret;
	ret = rts2db::AppDb::init ();
	if (ret)
		return ret;

	return 0;
}

int Rts2AirmasScale::doSetAirmass ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	float db_airmass_start;
	float db_airmass_end;
	EXEC SQL END DECLARE SECTION;

	for (db_airmass_start = ln_get_airmass (90, 750.0); db_airmass_start < max; db_airmass_start += steps)
	{
		db_airmass_end = db_airmass_start + steps;
		if (db_airmass_end > max)
			db_airmass_end = 1000;
		EXEC SQL
			INSERT INTO
				airmass_cal_images
				(
				air_airmass_start,
				air_airmass_end
				)
			VALUES
				(
				:db_airmass_start,
				:db_airmass_end
				);
		if (sqlca.sqlcode)
		{
			std::cerr << "Cannot insert to airmass_cal_images: " << sqlca.sqlerrm.sqlerrmc << std::endl;
			EXEC SQL ROLLBACK;
			return -1;
		}
	}
	EXEC SQL COMMIT;
	return 0;
}

int Rts2AirmasScale::doPrintAction ()
{
	std::cout << "Not implemented!" << std::endl;
	return -1;
}

int Rts2AirmasScale::doProcessing ()
{
	switch (action)
	{
		case setAirmass:
			return doSetAirmass ();
		case printAction:
			return doPrintAction ();
	}
	return -1;
}

int main (int argc, char **argv)
{
	Rts2AirmasScale app = Rts2AirmasScale (argc, argv);
	return app.run ();
}
