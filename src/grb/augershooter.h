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

#include "rts2db/devicedb.h"
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
class DevAugerShooter:public rts2db::DeviceDb
{
	public:
		DevAugerShooter (int in_argc, char **in_argv);
		virtual ~ DevAugerShooter (void);

		virtual int ready ()
		{
			return 0;
		}

		virtual int init ();
		void augerMessage ();
		void rejectedShower (double lastDate, double ra, double dec);
		void newShower (double lastDate, double ra, double dec);
		bool wasSeen (double lastDate, double ra, double dec);
	protected:
		virtual int processOption (int in_opt);

	private:
		ConnShooter * shootercnn;
		friend class ConnShooter;

		int port;
		const char *testParsing;

		rts2core::ValueTime *lastAugerSeen;
		rts2core::ValueTime *lastAugerDate;
		rts2core::ValueTime *lastAugerMessage;
		rts2core::ValueDouble *lastAugerRa;
		rts2core::ValueDouble *lastAugerDec;

		rts2core::ValueDouble *minEnergy;
		rts2core::ValueDouble *maxXmaxErr;
		rts2core::ValueDouble *maxEnergyDiv;
		rts2core::ValueDouble *maxGHChiDiv;
		rts2core::ValueDouble *minLineFitDiff;
		rts2core::ValueDouble *maxAxisDist;
		rts2core::ValueDouble *minRp;
		rts2core::ValueDouble *minChi0;
		rts2core::ValueDouble *maxSPDDiv;
		rts2core::ValueDouble *maxTimeDiv;
		rts2core::ValueDouble *maxTheta;

		rts2core::ValueInteger *maxTime;
		rts2core::ValueInteger *EyeId1;

		rts2core::ValueBool *triggeringEnabled;

   /*       second set of cuts         */
		rts2core::ValueInteger *EyeId2;
		rts2core::ValueDouble *minEnergy2;
		rts2core::ValueInteger *minPix2;
		rts2core::ValueDouble *maxAxisDist2;
		rts2core::ValueDouble *maxTimeDiff2;
		rts2core::ValueDouble *maxGHChiDiv2;
		rts2core::ValueDouble *maxLineFitDiv2;
		rts2core::ValueDouble *minViewAngle2;
 /*       second set of cuts - end   */

 /*       third set of cuts          */
		rts2core::ValueInteger *EyeId3;
		rts2core::ValueDouble *minEnergy3;
		rts2core::ValueDouble *maxXmaxErr3;
		rts2core::ValueDouble *maxEnergyDiv3;
		rts2core::ValueDouble *maxGHChiDiv3;
		rts2core::ValueDouble *maxLineFitDiv3;
		rts2core::ValueInteger *minPix3;
		rts2core::ValueDouble *maxAxisDist3;
		rts2core::ValueDouble *minRp3;
		rts2core::ValueDouble *minChi03;
		rts2core::ValueDouble *maxSPDDiv3;
		rts2core::ValueDouble *maxTimeDiv3;
 /*       third set of cuts - end    */

 /*       fourth set of cuts         */
		rts2core::ValueInteger *EyeId4;
		rts2core::ValueDouble *minNPix4;
		rts2core::ValueDouble *maxDGHChi2Improv4;
 /*       fourth set of cuts - end   */

 /*       fifth set of cuts         */
		rts2core::ValueInteger *EyeId5;
		rts2core::ValueDouble *XmaxEnergyShift5;
		rts2core::ValueDouble *XmaxEnergyLin5;
		rts2core::ValueDouble *XmaxErr5;
		rts2core::ValueDouble *Energy5;
		rts2core::ValueDouble *ChkovFrac5;
 /*       fifth set of cuts - end   */
};

}
#endif							 /*! __RTS2_AUGERSHOOTER__ */
