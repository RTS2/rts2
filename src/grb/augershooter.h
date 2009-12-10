/* 
 * Receive and reacts to Auger showers.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_AUGERSHOOTER__
#define __RTS2_AUGERSHOOTER__

#include "../utilsdb/rts2devicedb.h"
#include "connshooter.h"

#define RTS2_EVENT_AUGER_SHOWER   RTS2_LOCAL_EVENT + 700

namespace rts2grbd
{

class ConnShooter;

/**
 * Device class for auger shooter. Opens ShooterConn, waits for shooters and display their statistics.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DevAugerShooter:public Rts2DeviceDb
{
	public:
		DevAugerShooter (int in_argc, char **in_argv);
		virtual ~ DevAugerShooter (void);

		virtual int ready ()
		{
			return 0;
		}

		virtual int init ();
		int newShower (double lastDate, double ra, double dec);
		bool wasSeen (double lastDate, double ra, double dec);
	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

	private:
		ConnShooter * shootercnn;
		friend class ConnShooter;

		int port;

		Rts2ValueTime *lastAugerDate;
		Rts2ValueDouble *lastAugerRa;
		Rts2ValueDouble *lastAugerDec;

		Rts2ValueDouble *minEnergy;
		Rts2ValueDouble *maxXmaxErr;
		Rts2ValueDouble *maxEnergyDiv;
		Rts2ValueDouble *maxGHChiDiv;
		Rts2ValueDouble *minLineFitDiff;
		Rts2ValueDouble *maxAxisDist;
		Rts2ValueDouble *minRp;
		Rts2ValueDouble *minChi0;
		Rts2ValueDouble *maxSPDDiv;
		Rts2ValueDouble *maxTimeDiv;
		Rts2ValueDouble *maxTheta;

		Rts2ValueInteger *maxTime;
		Rts2ValueInteger *EyeId1;

		Rts2ValueBool *triggeringEnabled;

   /*       second set of cuts         */
		Rts2ValueInteger *EyeId2;
		Rts2ValueDouble *minEnergy2;
		Rts2ValueInteger *minPix2;
		Rts2ValueDouble *maxAxisDist2;
		Rts2ValueDouble *maxTimeDiff2;
		Rts2ValueDouble *maxGHChiDiv2;
		Rts2ValueDouble *maxLineFitDiv2;
		Rts2ValueDouble *minViewAngle2;
 /*       second set of cuts - end   */

 /*       third set of cuts          */
		Rts2ValueInteger *EyeId3;
		Rts2ValueDouble *minEnergy3;
		Rts2ValueDouble *maxXmaxErr3;
		Rts2ValueDouble *maxEnergyDiv3;
		Rts2ValueDouble *maxGHChiDiv3;
		Rts2ValueDouble *maxLineFitDiv3;
		Rts2ValueInteger *minPix3;
		Rts2ValueDouble *maxAxisDist3;
		Rts2ValueDouble *minRp3;
		Rts2ValueDouble *minChi03;
		Rts2ValueDouble *maxSPDDiv3;
		Rts2ValueDouble *maxTimeDiv3;
 /*       third set of cuts - end    */
};

}
#endif							 /*! __RTS2_AUGERSHOOTER__ */
