/*
 * Scheduling body.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "../utilsdb/rts2appdb.h"
#include "../utils/rts2config.h"

#include "rts2schedule.h"

class Rts2ScheduleApp: public Rts2AppDb
{
	private:
		Rts2Config *config;

	protected:
		virtual void usage ();
		virtual void help ();

	public:
		Rts2ScheduleApp (int argc, char ** argv);
		virtual ~Rts2ScheduleApp (void);

		virtual int doProcessing ();
};


void
Rts2ScheduleApp::usage ()
{
	std::cout << "\t" << getAppName () << std::endl;
}


void
Rts2ScheduleApp::help ()
{
	std::cout << "Create schedule for given night. The programme uses genetic algorithm scheduling, described at \
http://rts-2.sf.net/scheduling.pdf." << std::endl
                 << "You are free to experiment with various settings to create optimal observing schedule" << std::endl;
	Rts2AppDb::help ();
}


Rts2ScheduleApp::Rts2ScheduleApp (int argc, char ** argv): Rts2AppDb (argc, argv)
{

}


Rts2ScheduleApp::~Rts2ScheduleApp (void)
{
}


int
Rts2ScheduleApp::doProcessing ()
{
	struct ln_lnlat_posn *observer = Rts2Config::instance()->getObserver ();
	double jd = ln_get_julian_from_sys ();

	Rts2Schedule sched = Rts2Schedule (jd, jd + 1, observer);
	sched.constructSchedule ();

	std::cout << sched << std::endl;
	return 0;
}


int
main (int argc, char ** argv)
{
	Rts2ScheduleApp app = Rts2ScheduleApp (argc, argv);
	return app.run ();
}
