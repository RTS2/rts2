/* 
 * File for testing of the configuration routines.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include <iostream>
#include <assert.h>
#include <errno.h>
#include <libnova/libnova.h>

#include "utilsfunc.h"

#include "app.h"
#include "rts2config.h"
#include "expander.h"

/**
 * Test application.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2TestApp:public rts2core::App
{
	public:
		Rts2TestApp (int in_argc, char **in_argv):rts2core::App (in_argc, in_argv) {}
		virtual int run ();
};

int Rts2TestApp::run ()
{
	double value;
	int i_value;

	std::string buf;

	i_value = init ();
	if (i_value)
		return i_value;

	assert (mkpath ("test/test1/test2/test3/", 0777) == -1);
	assert (mkpath ("aa/bb/cc/dd", 0777) == 0);

	Rts2Config *conf = Rts2Config::instance ();

	std::cout << "ret " << conf->loadFile ("test.ini") << std::endl;

	struct ln_lnlat_posn *observer;
	observer = conf->getObserver ();
	std::cout << "observatory long " << observer->lng << std::endl;
	std::cout << "observatory lat " << observer->lat << std::endl;
	std::cout << " val " << value << std::endl;
	std::cout << "C1 name ret: " << conf->getString ("C1", "name", buf);
	std::cout << " val " << buf << std::endl;
	std::cout << "centrald day_horizont ret: " << conf->getDouble ("centrald", "day_horizont", value);
	std::cout << " val " << value << std::endl;
	std::cout << "sbig blocked by C0: " << conf->blockDevice ("sbig", "C0") << std::endl;
	std::cout << "sbig blocked by FSBIG: " << conf->blockDevice ("sbig", "FSBIG") << std::endl;
	std::cout << "C1 blocked by FSBIG: " << conf->blockDevice ("C1", "FSBIG") << std::endl;
	std::cout << "PHOT blocked by FSBIG: " << conf->blockDevice ("PHOT", "FSBIG") << std::endl;
	std::cout << "centrald night_horizont ret: " << conf->getDouble ("centrald", "night_horizont", value);
	std::cout << " val " << value << std::endl;
	std::cout << "hete dark_frequency ret: " << conf->getInteger ("hete", "dark_frequency", i_value);
	std::cout << " val " << i_value << std::endl;

	std::cout << "CNF1 script ret: " << conf->getString ("CNF1", "script", buf);
	std::cout << " val " << buf << std::endl;
	std::cout << "horizon ret: " << conf->getString ("observatory", "horizont", buf) << std::endl;

	std::cout << std::endl <<
		"************************ CONFIG FILE test.ini DUMP **********************"
		<< std::endl;

	// step throught sections..
	for (Rts2Config::iterator iter = conf->begin (); iter != conf->end ();
		iter++)
	{
		Rts2ConfigSection *sect = *iter;
		std::cout << std::endl << "[" << sect->getName () << "]" << std::endl;
		for (Rts2ConfigSection::iterator viter = sect->begin ();
			viter != sect->end (); viter++)
		std::cout << *viter << std::endl;
	}

	// now do test expansions..
	rts2core::Expander *exp = new rts2core::Expander ();
	std::cout << "%Z%D:%y-%m-%dT%H:%M:%S:%s:%u: " << exp->expand ("%Z%D:%y-%m-%dT%H:%M:%S:%s:%u") << std::endl;
	delete exp;

	delete conf;

	// test array counting
	const char *endp;
	std::vector <int> pa = parseRange (":4,7:9,11:", 16, endp);
	for (std::vector <int>::iterator iter = pa.begin (); iter != pa.end (); iter++)
		std::cout << " " << (*iter);
	std::cout << std::endl;

	pa = parseRange ("4,7,11:", 16, endp);
	for (std::vector <int>::iterator iter = pa.begin (); iter != pa.end (); iter++)
		std::cout << " " << (*iter);
	std::cout << std::endl;

	pa = parseRange ("4", 16, endp);
	for (std::vector <int>::iterator iter = pa.begin (); iter != pa.end (); iter++)
		std::cout << " " << (*iter);
	std::cout << std::endl;

	pa = parseRange ("", 16, endp);
	for (std::vector <int>::iterator iter = pa.begin (); iter != pa.end (); iter++)
		std::cout << " " << (*iter);
	std::cout << std::endl;

	pa = parseRange (":", 16, endp);
	for (std::vector <int>::iterator iter = pa.begin (); iter != pa.end (); iter++)
		std::cout << " " << (*iter);
	std::cout << std::endl;

	return 0;
}

int main (int argc, char **argv)
{
	Rts2TestApp app (argc, argv);
	return app.run ();
}
