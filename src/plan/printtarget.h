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

#include "../../lib/rts2db/constraints.h"
#include "../../lib/rts2db/rts2appdb.h"
#include "../../lib/rts2db/rts2camlist.h"
#include "../../lib/rts2db/target.h"
#include "../../lib/rts2db/observationset.h"
#include "rts2config.h"
#include "rts2format.h"
#include "libnova_cpp.h"
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
		void printScripts (rts2db::Target *target, const char *pref);
		void printTarget (rts2db::Target *target);
		void printTargetGNUplot (rts2db::Target *target);
		void printTargetGNUBonus (rts2db::Target *target);
		void printTargetDS9 (rts2db::Target *target);

		struct ln_lnlat_posn *obs;

	private:
		Rts2CamList cameras;
		std::vector <std::string> scriptCameras;
		int printExtended;
		bool printCalTargets;
		bool printObservations;
		bool printSatisfied;
		bool printViolated;
		double printVisible;
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

		const char *constraintFile;
		rts2db::Constraints *constraints;

		void setGnuplotType (const char *type);
};

}
