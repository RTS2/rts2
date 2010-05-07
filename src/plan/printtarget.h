/* 
 * Prints informations about target.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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
#include "../utilsdb/rts2camlist.h"
#include "../utilsdb/target.h"
#include "../utilsdb/observationset.h"
#include "../utils/rts2config.h"
#include "../utils/rts2format.h"
#include "../utils/libnova_cpp.h"
#include "script.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>

namespace rts2plan {

/**
 * Print target and its associates options. Handles command options
 * parsing to display various target information.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PrintTarget:public Rts2AppDb
{
	public:
		PrintTarget (int argc, char **argv);
		virtual ~ PrintTarget (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
	
		int printTargets (rts2db::TargetSet & set);
		void printScripts (Target *target, const char *pref);
		void printTarget (Target *target);
		void printTargetGNUplot (Target *target);
		void printTargetGNUBonus (Target *target);
		void printTargetDS9 (Target *target);

		struct ln_lnlat_posn *obs;

	private:
		Rts2CamList cameras;
		int printExtended;
		bool printCalTargets;
		bool printObservations;
		int printImages;
		int printCounts;
		int printGNUplot;
		bool printAuger;
		bool printDS9;
		bool addMoon;
		bool addHorizon;

		double JD;

		double jd_start;
		double step;

		// plot begin and end (hours)
		double gbeg, gend;

		double airmd;
};

}
